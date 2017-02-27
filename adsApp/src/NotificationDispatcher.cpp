/**
   Copyright (c) 2015 Beckhoff Automation GmbH & Co. KG

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
 */

#include "NotificationDispatcher.h"
#include "Log.h"

NotificationDispatcher::NotificationDispatcher(AmsProxy& __proxy, VirtualConnection __conn)
    : conn(__conn),
    ring(4 * 1024 * 1024),
    proxy(__proxy),
    thread(&NotificationDispatcher::Run, this)
{}

NotificationDispatcher::~NotificationDispatcher()
{
    sem.Close();
    thread.join();
}

bool NotificationDispatcher::operator<(const NotificationDispatcher& ref) const
{
    return conn.second < ref.conn.second;
}

void NotificationDispatcher::Emplace(uint32_t hNotify, Notification& notification)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    notifications.emplace(hNotify, notification);
}

long NotificationDispatcher::Erase(uint32_t hNotify, uint32_t tmms)
{
    const auto status = proxy.DeleteNotification(conn.second, hNotify, tmms, conn.first);
    std::lock_guard<std::recursive_mutex> lock(mutex);
    notifications.erase(hNotify);
    return status;
}

void NotificationDispatcher::Run()
{
    while (sem.Wait()) {
        const auto length = ring.ReadFromLittleEndian<uint32_t>();
        (void)length;
        const auto numStamps = ring.ReadFromLittleEndian<uint32_t>();
        for (uint32_t stamp = 0; stamp < numStamps; ++stamp) {
            const auto timestamp = ring.ReadFromLittleEndian<uint64_t>();
            const auto numSamples = ring.ReadFromLittleEndian<uint32_t>();
            for (uint32_t sample = 0; sample < numSamples; ++sample) {
                const auto hNotify = ring.ReadFromLittleEndian<uint32_t>();
                const auto size = ring.ReadFromLittleEndian<uint32_t>();
                std::lock_guard<std::recursive_mutex> lock(mutex);
                auto it = notifications.find(hNotify);
                if (it != notifications.end()) {
                    auto& notification = it->second;
                    if (size != notification.Size()) {
                        LOG_WARN("Notification sample size: " << size << " doesn't match: " << notification.Size());
                        ring.Read(size);
                        return;
                    }
                    notification.Notify(timestamp, ring);
                } else {
                    ring.Read(size);
                }
            }
        }
    }
}
