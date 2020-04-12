// SoundplaneDriver.cpp
//
// Returns raw data frames from the Soundplane.  The frames are reclocked if needed (TODO)
// to reconstruct a steady sample rate.
//
// Two threads are used to do this work.  A grab thread maintains a stream of
// low-latency isochronous transfers. A process thread looks through the buffers
// specified by these transfers every ms or so.  When new frames of data arrive,
// the process thread reclocks them and pushes them to a ring buffer where they
// can be read by clients.

#include "LibusbSoundplaneDriver.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "ThreadUtility.h"

namespace {

constexpr int kInterfaceNumber = 0;

template<typename GlitchCallback, typename SuccessCallback>
class AnomalyFilter {
public:
    AnomalyFilter(GlitchCallback glitchCallback, SuccessCallback successCallback) :
        mGlitchCallback(std::move(glitchCallback)),
        mSuccessCallback(std::move(successCallback)) {}

    void operator()(const SensorFrame &frame) {
        if (mStarted) {
            float df = frameDiff(mPreviousFrame, frame);
            if (df < kMaxFrameDiff) {
                // We are OK, the data gets out normally
                mSuccessCallback(frame);
            } else {
                // Possible sensor glitch.  also occurs when changing carriers.
                mGlitchCallback(mFrameCtr, df, mPreviousFrame, frame);
                reset();
            }
        } else if (mFrameCtr > kSoundplaneStartupFrames) {
            mStarted = true;
            mSuccessCallback(frame);
        } else {
            // Wait for initialization
        }
        mFrameCtr++;

        mPreviousFrame = frame;
    }

    void reset() {
        mFrameCtr = 0;
        mStarted = false;
    }

private:
    static const unsigned kSoundplaneStartupFrames = 250;

    SensorFrame mPreviousFrame{};
    unsigned mFrameCtr = 0;
    bool mStarted = false;
    GlitchCallback mGlitchCallback;
    SuccessCallback mSuccessCallback;
};

template<typename GlitchCallback, typename SuccessCallback>
AnomalyFilter<GlitchCallback, SuccessCallback> makeAnomalyFilter(
    GlitchCallback glitchCallback, SuccessCallback successCallback) {
    return AnomalyFilter<GlitchCallback, SuccessCallback>(
        std::move(glitchCallback), std::move(successCallback));
}

bool libusbTransferStatusIsFatal(libusb_transfer_status error) {
    return
        error == LIBUSB_TRANSFER_STALL ||
        error == LIBUSB_TRANSFER_NO_DEVICE ||
        error == LIBUSB_TRANSFER_OVERFLOW;
}

}


std::unique_ptr<SoundplaneDriver> SoundplaneDriver::create(SoundplaneDriverListener &m) {
    return std::unique_ptr<LibusbSoundplaneDriver>(new LibusbSoundplaneDriver(m));
}


LibusbSoundplaneDriver::LibusbSoundplaneDriver(SoundplaneDriverListener &listener) :
    mState(kNoDevice),
    mQuitting(false),
    mListener(listener),
    mSetCarriersRequest(nullptr),
    mEnableCarriersRequest(nullptr) {
    if (libusb_init(&mLibusbContext) < 0) {
        throw std::runtime_error("LibusbSoundplaneDriver::Failed to initialize libusb");
    }
#ifdef __COBALT__
    pthread_mutex_init(&mMutex,0);
    pthread_cond_init(&mCondition,0);
#endif
}

LibusbSoundplaneDriver::~LibusbSoundplaneDriver() {
    // This causes getDeviceState to return kDeviceIsTerminating
    mQuitting.store(true, std::memory_order_release);
#ifdef __COBALT__
    pthread_cond_broadcast(&mCondition);
#else
    mCondition.notify_one();
#endif
    mProcessThread.join();

    delete mEnableCarriersRequest.load(std::memory_order_acquire);
    delete mSetCarriersRequest.load(std::memory_order_acquire);

    libusb_exit(mLibusbContext);
}

void* sppt(void* a) {
	LibusbSoundplaneDriver *pThis=static_cast<LibusbSoundplaneDriver*>(a);
	pThis->processThread();
	return nullptr;
}

void LibusbSoundplaneDriver::start() {
    // create device grab thread
#if __COBALT__
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_t ph = mProcessThread.native_handle();
    pthread_create(&ph, &attr, sppt, this);
    pthread_attr_destroy(&attr);
#else
    mProcessThread = std::thread(&LibusbSoundplaneDriver::processThread, this);
#endif
    SetPriorityRealtimeAudio(mProcessThread.native_handle());

}

int LibusbSoundplaneDriver::getDeviceState() const {
    return mState;
    //TODO - check this
//	return mQuitting.load(std::memory_order_acquire) ?
//		kDeviceIsTerminating :
//		mState.load(std::memory_order_acquire);
}

uint16_t LibusbSoundplaneDriver::getFirmwareVersion() const {
    return mFirmwareVersion.load(std::memory_order_acquire);
}

std::string LibusbSoundplaneDriver::getSerialNumberString() const {
    const std::array<unsigned char, 64> serialNumber = mSerialNumber.load();
    return std::string(reinterpret_cast<const char *>(serialNumber.data()));
}

const unsigned char *LibusbSoundplaneDriver::getCarriers() const {
    // FIXME: Change this interface to specify the size of the data
    return mCurrentCarriers.data();
}

void LibusbSoundplaneDriver::setCarriers(const Carriers &carriers) {
    auto *const sentCarriers = new Carriers(carriers);
    mCurrentCarriers = carriers;
    delete mSetCarriersRequest.exchange(sentCarriers, std::memory_order_release);
}

void LibusbSoundplaneDriver::enableCarriers(unsigned long mask) {
    delete mEnableCarriersRequest.exchange(
        new unsigned long(mask), std::memory_order_release);
}

void LibusbSoundplaneDriver::processThreadControlTransferCallback(struct libusb_transfer *xfr) {
    auto *driver = static_cast<LibusbSoundplaneDriver *>(xfr->user_data);
    driver->mOutstandingTransfers--;
}

libusb_error LibusbSoundplaneDriver::processThreadSendControl(
    libusb_device_handle *device,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    const unsigned char *data,
    size_t dataSize) {
    if (processThreadShouldStopTransfers()) {
        return LIBUSB_ERROR_OTHER;
    }

    auto *buf = static_cast<unsigned char *>(malloc(LIBUSB_CONTROL_SETUP_SIZE + dataSize));
    struct libusb_transfer *transfer;

    if (!buf) {
        return LIBUSB_ERROR_NO_MEM;
    }

    memcpy(buf + LIBUSB_CONTROL_SETUP_SIZE, data, dataSize);

    transfer = libusb_alloc_transfer(0);
    if (!transfer) {
        free(buf);
        return LIBUSB_ERROR_NO_MEM;
    }

    static constexpr auto kCtrlOut = uint8_t(LIBUSB_REQUEST_TYPE_VENDOR) | uint8_t(LIBUSB_ENDPOINT_OUT);
    libusb_fill_control_setup(buf, kCtrlOut, request, value, index, static_cast<uint16_t>(dataSize));
    libusb_fill_control_transfer(transfer, device, buf, &LibusbSoundplaneDriver::processThreadControlTransferCallback, this, 1000);

    transfer->flags = LIBUSB_TRANSFER_SHORT_NOT_OK
                      | LIBUSB_TRANSFER_FREE_BUFFER
                      | LIBUSB_TRANSFER_FREE_TRANSFER;
    const auto result = static_cast<libusb_error>(libusb_submit_transfer(transfer));
    if (result >= 0) {
        mOutstandingTransfers++;
    }
    return result;
}

bool LibusbSoundplaneDriver::processThreadWait(int ms) {
#ifdef __COBALT__
    pthread_mutex_lock(&mMutex);
    unsigned long long t = ms * 1000;
    struct timespec ts = { static_cast<time_t>(t/1000000ULL), static_cast<long>(1000ULL*(t%1000000ULL)) };

    clock_gettime(CLOCK_REALTIME, &ts);

    t += (ts.tv_nsec/1000ULL);
    ts.tv_nsec = 0;
    ts.tv_sec += t/1000000ULL;
    ts.tv_nsec += 1000ULL*(t%1000000ULL);

    pthread_cond_timedwait(&mCondition,&mMutex,&ts);
    pthread_mutex_unlock(&mMutex);
#else
    std::unique_lock<std::mutex> lock(mMutex);
    mCondition.wait_for(lock, std::chrono::milliseconds(ms));
#endif
    return !mQuitting.load(std::memory_order_acquire);
}

bool LibusbSoundplaneDriver::processThreadOpenDevice(LibusbClaimedDevice &outDevice) {
    for (;;) {
        libusb_device_handle *handle = libusb_open_device_with_vid_pid(
            mLibusbContext, kSoundplaneUSBVendor, kSoundplaneUSBProduct);
        LibusbClaimedDevice result(LibusbDevice(handle), kInterfaceNumber);
        if (result) {
            std::swap(result, outDevice);
            return true;
        }
        if (!processThreadWait(1000)) {
            return false;
        }
    }
}

bool LibusbSoundplaneDriver::processThreadGetDeviceInfo(libusb_device_handle *device) {
    libusb_device_descriptor descriptor{};
    if (libusb_get_device_descriptor(libusb_get_device(device), &descriptor) < 0) {
        fprintf(stderr, "Failed to get the device descriptor\n");
        return false;
    }

    std::array<unsigned char, 64> buffer{};
    int len = libusb_get_string_descriptor_ascii(device, descriptor.iSerialNumber, buffer.data(), buffer.size());
    if (len < 0) {
        fprintf(stderr, "Failed to get the device serial number\n");
        return false;
    }
    buffer[len] = 0;

    mFirmwareVersion.store(descriptor.bcdDevice, std::memory_order_release);
    mSerialNumber = buffer;

    return true;
}

bool LibusbSoundplaneDriver::processThreadFillTransferInformation(
    Transfers &transfers,
    LibusbUnpacker *unpacker,
    libusb_device_handle *device) {
    libusb_config_descriptor *descriptor;
    if (libusb_get_active_config_descriptor(libusb_get_device(device), &descriptor) < 0) {
        fprintf(stderr, "Failed to get device debug information\n");
        return false;
    }

    printf("Available Bus Power: %d mA\n", 2 * static_cast<int>(descriptor->MaxPower));

    if (descriptor->bNumInterfaces <= kInterfaceNumber) {
        fprintf(stderr, "No available interfaces: %d\n", descriptor->bNumInterfaces);
        return false;
    }
    const struct libusb_interface &interface = descriptor->interface[kInterfaceNumber];
    if (interface.num_altsetting <= kSoundplaneAlternateSetting) {
        fprintf(stderr, "Desired alt setting %d is not available\n", kSoundplaneAlternateSetting);
        return false;
    }
    const struct libusb_interface_descriptor &interfaceDescriptor = interface.altsetting[kSoundplaneAlternateSetting];
    if (interfaceDescriptor.bNumEndpoints < kSoundplaneANumEndpoints) {
        fprintf(
            stderr,
            "Alt setting %d has too few endpoints (has %d, needs %d)\n",
            kSoundplaneAlternateSetting,
            interfaceDescriptor.bNumEndpoints,
            kSoundplaneANumEndpoints);
        return false;
    }
    for (int i = 0; i < transfers.size(); i++) {
        const struct libusb_endpoint_descriptor &endpoint = interfaceDescriptor.endpoint[i];
        auto &transfersForEndpoint = transfers[i];
        for (int j = 0; j < transfersForEndpoint.size(); j++) {
            auto &transfer = transfersForEndpoint[j];
            transfer.endpointId = i;
            transfer.endpointAddress = endpoint.bEndpointAddress;
            transfer.device = device;
            transfer.parent = this;
            transfer.unpacker = unpacker;
            // Divide the transfers into groups of kInFlightMultiplier. Within
            // each group, each transfer points to the previous one, except for
            // the first, which points to the last one. With this scheme, the
            // nextTransfer pointers form cycles of length kInFlightMultiplier.
            //
            // @see The nextTransfer member declaration
            const bool isAtStartOfCycle = j % kInFlightMultiplier == 0;
            transfer.nextTransfer = &transfersForEndpoint[j + (isAtStartOfCycle ? kInFlightMultiplier - 1 : -1)];
        }
    }

    return true;
}

bool LibusbSoundplaneDriver::processThreadSetDeviceState(int newState) {
    mState.store(newState, std::memory_order_release);
//	mListener->deviceStateChanged(*this, newState);
    return !mQuitting.load(std::memory_order_acquire);
}

bool LibusbSoundplaneDriver::processThreadSelectIsochronousInterface(libusb_device_handle *device) const {
    if (libusb_set_interface_alt_setting(device, kInterfaceNumber, kSoundplaneAlternateSetting) < 0) {
        fprintf(stderr, "Failed to select alternate setting on the Soundplane\n");
        return false;
    }
    return true;
}

bool LibusbSoundplaneDriver::processThreadShouldStopTransfers() const {
    return mUsbFailed || mQuitting.load(std::memory_order_acquire);
}

bool LibusbSoundplaneDriver::processThreadScheduleTransfer(Transfer &transfer) {
    if (processThreadShouldStopTransfers()) {
        return false;
    }

    libusb_fill_iso_transfer(
        transfer.transfer,
        transfer.device,
        static_cast<unsigned char>(transfer.endpointAddress),
        reinterpret_cast<unsigned char *>(transfer.packets),
        sizeof(transfer.packets),
        LibusbSoundplaneDriver::Transfer::numPackets(),
        processThreadTransferCallbackStatic,
        &transfer,
        1000);
    libusb_set_iso_packet_lengths(
        transfer.transfer,
        sizeof(transfer.packets) / LibusbSoundplaneDriver::Transfer::numPackets());

    const auto result = libusb_submit_transfer(transfer.transfer);
    if (result < 0) {
        fprintf(stderr, "Failed to submit USB transfer: %s\n", libusb_error_name(result));
        return false;
    } else {
        mOutstandingTransfers++;
        return true;
    }
}

bool LibusbSoundplaneDriver::processThreadScheduleInitialTransfers(
    Transfers &transfers) {
    for (int endpoint = 0; endpoint < kSoundplaneANumEndpoints; endpoint++) {
        for (int buffer = 0; buffer < kSoundplaneABuffersInFlight; buffer++) {
            auto &transfer = transfers[endpoint][buffer * kInFlightMultiplier];
            if (!processThreadScheduleTransfer(transfer)) {
                return false;
            }
        }
    }
    return true;
}

void LibusbSoundplaneDriver::processThreadTransferCallbackStatic(struct libusb_transfer *xfr) {
    auto *transfer = static_cast<Transfer *>(xfr->user_data);
    transfer->parent->mOutstandingTransfers--;
    transfer->parent->processThreadTransferCallback(*transfer);
}

void LibusbSoundplaneDriver::processThreadTransferCallback(Transfer &transfer) {
    // Check if the transfer was successful
    const auto status = transfer.transfer->status;
    if (status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "Failed USB transfer: %s\n", libusb_error_name(transfer.transfer->status));
        if (libusbTransferStatusIsFatal(status)) {
            fprintf(stderr, "(Transfer status caused device reconnect)\n");
            mUsbFailed = true;
            return;
        }
    }

    // Report kDeviceHasIsochSync if appropriate
    if (mState.load(std::memory_order_acquire) == kDeviceConnected) {
        processThreadSetDeviceState(kDeviceHasIsochSync);
    }

    bool isShort=false;
    unsigned numPkt = transfer.transfer->num_iso_packets;
    for(unsigned i = 0; !isShort && i <(transfer.transfer->num_iso_packets);i++ ) {
        isShort=(transfer.transfer->iso_packet_desc[i].actual_length)==0;
        if(isShort) {
          numPkt = i;
        }
    }

    if(numPkt>0) {
            transfer.unpacker->gotTransfer(
            transfer.endpointId,
            transfer.packets,
            numPkt);
    }

    // Schedule another transfer
    Transfer &nextTransfer = *transfer.nextTransfer;
    if (!processThreadScheduleTransfer(nextTransfer)) {
        mUsbFailed = true;
        return;
    }
}

libusb_error LibusbSoundplaneDriver::processThreadSetCarriers(
    libusb_device_handle *device, const unsigned char *carriers, size_t carriersSize) {
    return processThreadSendControl(
        device,
        kRequestMask,
        0,
        kRequestCarriersIndex,
        carriers,
        carriersSize);
}

bool LibusbSoundplaneDriver::processThreadHandleRequests(libusb_device_handle *device) {
    const auto carrierMask = mEnableCarriersRequest.exchange(nullptr, std::memory_order_acquire);
    if (carrierMask) {
        unsigned long mask = *carrierMask;
        processThreadSendControl(
            device,
            kRequestMask,
            static_cast<uint16_t>(mask >> 16),
            static_cast<uint16_t>(mask),
            nullptr,
            0);
        delete carrierMask;
    }
    const auto carriers = mSetCarriersRequest.exchange(nullptr, std::memory_order_acquire);
    if (carriers) {
        processThreadSetCarriers(device, carriers->data(), carriers->size());
        delete carriers;
    }
    return carrierMask || carriers;
}

void LibusbSoundplaneDriver::processThread() {
    // Each iteration of this loop is one cycle of finding a Soundplane device,
    // using it, and the device going away.
    while (!mQuitting.load(std::memory_order_acquire)) {
        mUsbFailed = false;
        mOutstandingTransfers = 0;

        Transfers transfers;
        LibusbClaimedDevice handle;
        auto anomalyFilter = makeAnomalyFilter(
            [this](int frameCtr, float df, const SensorFrame &previousFrame, const SensorFrame &frame) {

                snprintf(mErrorBuf, kMaxErrorStringSize, "frame(%d) ", frameCtr);
                mListener.onError(kDevDataDiffTooLarge, mErrorBuf);
            },
            [this](const SensorFrame &frame) {
                mListener.onFrame(frame);
            });
        LibusbUnpacker unpacker(anomalyFilter);

        bool success =
            processThreadOpenDevice(handle) &&
            processThreadGetDeviceInfo(handle.get()) &&
            processThreadSelectIsochronousInterface(handle.get()) &&
            processThreadFillTransferInformation(transfers, &unpacker, handle.get()) &&
            processThreadSetDeviceState(kDeviceConnected) &&
            processThreadScheduleInitialTransfers(transfers);

        if (!success) continue;

        mListener.onStartup();
        // FIXME: Handle debugger interruptions

        /// Run the main event loop
        while (!processThreadShouldStopTransfers() || mOutstandingTransfers != 0) {
            if (libusb_handle_events(mLibusbContext) != LIBUSB_SUCCESS) {
                fprintf(stderr, "Libusb error!\n");
                break;
            }
            if (mState.load(std::memory_order_acquire) == kDeviceHasIsochSync &&
                processThreadHandleRequests(handle.get())) {
                // Wait for data to settle after setting carriers
                anomalyFilter.reset();
            }
        }

        mListener.onClose();
        if (!processThreadSetDeviceState(kNoDevice)) continue;
    }


    //TODO processThreadSetDeviceState(kDeviceIsTerminating);
}


int LibusbSoundplaneDriver::getSerialNumber() const {
    const auto state = getDeviceState();
    if (state == kDeviceConnected || state == kDeviceHasIsochSync) {
        const auto serialString = getSerialNumberString();
        try {
            return stoi(serialString);
        }
        catch (...) {
            return 0;
        }
    }
    return 0;
}
