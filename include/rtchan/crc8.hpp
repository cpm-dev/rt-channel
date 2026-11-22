#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace rtchan {

// CRC-8 SAE J1850: polynomial 0x1D, initial 0xFF, xor-out 0xFF.
constexpr std::uint8_t crc8_sae_j1850(
    std::span<const std::uint8_t> bytes) noexcept {
    std::uint8_t crc = 0xFF;

    for (const std::uint8_t byte : bytes) {
        crc ^= byte;
        for (std::size_t bit = 0; bit < 8; ++bit) {
            if ((crc & 0x80u) != 0) {
                crc = static_cast<std::uint8_t>((crc << 1) ^ 0x1D);
            } else {
                crc = static_cast<std::uint8_t>(crc << 1);
            }
        }
    }

    return static_cast<std::uint8_t>(crc ^ 0xFF);
}

inline constexpr std::uint8_t kCanFrameChecksumIndex = 7;

// Writes the checksum over bytes 0–6 into byte 7 of a classic 8-byte CAN frame.
constexpr bool set_can_frame_crc(std::span<std::uint8_t, 8> frame) noexcept {
    frame[kCanFrameChecksumIndex] =
        crc8_sae_j1850(frame.first(kCanFrameChecksumIndex));
    return true;
}

// Verifies byte 7 against a CRC calculated over bytes 0–6.
constexpr bool verify_can_frame_crc(std::span<const std::uint8_t, 8> frame) noexcept {
    return frame[kCanFrameChecksumIndex] ==
           crc8_sae_j1850(frame.first(kCanFrameChecksumIndex));
}

}  // namespace rtchan
