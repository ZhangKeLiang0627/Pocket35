#include <NimBLEDevice.h>
#include "kt6368a_profile.h"
#include "../hal_board.h"

namespace
{
    struct BleContext
    {
        NimBLEScan* scan = nullptr;
        NimBLEClient* client = nullptr;
        NimBLERemoteCharacteristic* dataChar = nullptr;
        NimBLERemoteCharacteristic* nameChar = nullptr;
        NimBLEAddress targetAddress;
        std::string targetName;
        bool hasTarget = false;
        bool shouldConnect = false;
        bool isSubscribed = false;
        uint32_t lastAttemptMs = 0;
    } context;

    void handleNotify(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify)
    {
        HAL::BleReportReceived(data, length);
    }

    class ScanCallbacks : public NimBLEScanCallbacks
    {
        void onResult(const NimBLEAdvertisedDevice* device) override
        {
            if (!device->haveName())
            {
                return;
            }

            const std::string name = device->getName();
            if (name.rfind(KT6368A::kNamePrefix, 0) != 0)
            {
                return;
            }

            auto& peer = HAL::GetBlePeer();
            peer.name = name;
            peer.address = device->getAddress().toString();
            peer.rssi = device->getRSSI();

            context.targetAddress = device->getAddress();
            context.targetName = name;
            context.hasTarget = true;
            context.shouldConnect = true;

            HAL::BleReportState(BLE::State::connecting);
        }
    };

    class ClientCallbacks : public NimBLEClientCallbacks
    {
        void onConnect(NimBLEClient* client) override
        {
            HAL::BleReportState(BLE::State::connected);
            HAL::GetBleStats().mtu = client->getMTU();
        }

        void onDisconnect(NimBLEClient* client, int reason) override
        {
            context.isSubscribed = false;
            context.dataChar = nullptr;
            context.nameChar = nullptr;
            context.lastAttemptMs = HAL::Millis();
            HAL::BleReportState(BLE::State::idle);
        }

        void onMTUChange(NimBLEClient* client, uint16_t mtu) override { HAL::GetBleStats().mtu = mtu; }
    };
} // namespace

void HAL_Board::bleInit()
{
    NimBLEDevice::init("Pocket35");
    NimBLEDevice::setMTU(KT6368A::kPreferredMtu);
}

void HAL_Board::bleStartScan()
{
    if (context.scan == nullptr)
    {
        context.scan = NimBLEDevice::getScan();
        context.scan->setScanCallbacks(new ScanCallbacks(), false);
        context.scan->setActiveScan(true);
        context.scan->setInterval(100);
        context.scan->setWindow(80);
        context.scan->setDuplicateFilter(0);
    }

    context.scan->clearResults();
    context.scan->start(KT6368A::kScanDurationMs, false);
    setBleState(BLE::State::scanning);
}

void HAL_Board::bleStopScan()
{
    if (context.scan != nullptr)
    {
        context.scan->stop();
    }

    if (!bleIsConnected())
    {
        setBleState(BLE::State::idle);
    }
}

void HAL_Board::bleConnect()
{
    if (context.hasTarget)
    {
        context.shouldConnect = true;
        context.lastAttemptMs = 0;
    }
}

void HAL_Board::bleDisconnect()
{
    context.shouldConnect = false;

    if (context.client != nullptr)
    {
        context.client->disconnect();
    }
}

bool HAL_Board::bleIsConnected() { return context.client != nullptr && context.client->isConnected(); }

bool HAL_Board::bleSend(const uint8_t* data, size_t length)
{
    if (context.dataChar == nullptr || !bleIsConnected())
    {
        return false;
    }

    if (length > KT6368A::kMaxPacketBytes)
    {
        return false;
    }

    if (!context.dataChar->writeValue(data, length, false))
    {
        return false;
    }

    _ble_stats.txPackets++;
    _ble_stats.txBytes += length;
    return true;
}

void HAL_Board::bleUpdate()
{
    const uint32_t now = Millis();

    if (context.client != nullptr && !context.client->isConnected())
    {
        NimBLEDevice::deleteClient(context.client);
        context.client = nullptr;
        context.isSubscribed = false;
        context.dataChar = nullptr;
        context.nameChar = nullptr;
        setBleState(BLE::State::idle);
    }

    if (context.shouldConnect && context.client == nullptr &&
        (now - context.lastAttemptMs) > KT6368A::kReconnectDelayMs)
    {
        context.shouldConnect = false;
        context.lastAttemptMs = now;
        setBleState(BLE::State::connecting);

        if (context.scan != nullptr && context.scan->isScanning())
        {
            context.scan->stop();
        }

        context.client = NimBLEDevice::createClient();
        if (context.client == nullptr)
        {
            setBleState(BLE::State::idle);
            return;
        }

        context.client->setClientCallbacks(new ClientCallbacks(), true);

        if (!context.client->connect(context.targetAddress))
        {
            NimBLEDevice::deleteClient(context.client);
            context.client = nullptr;
            setBleState(BLE::State::idle);
        }
    }

    if (context.client != nullptr && context.client->isConnected() && !context.isSubscribed)
    {
        auto* service = context.client->getService(KT6368A::kServiceUuid);
        if (service == nullptr)
        {
            return;
        }

        context.dataChar = service->getCharacteristic(KT6368A::kDataCharUuid);
        context.nameChar = service->getCharacteristic(KT6368A::kNameCharUuid);

        if (context.dataChar != nullptr && context.dataChar->canNotify())
        {
            context.dataChar->subscribe(true, handleNotify);
        }

        if (context.nameChar != nullptr && context.nameChar->canNotify())
        {
            context.nameChar->subscribe(true, handleNotify);
        }

        context.isSubscribed = true;
        _ble_stats.mtu = context.client->getMTU();
    }
}
