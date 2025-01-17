/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "private/backend/Driver.h"
#include "private/backend/CommandStream.h"

#include "DriverBase.h"

#include <math/half.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>

#include <backend/BufferDescriptor.h>
#include <backend/PixelBufferDescriptor.h>

#include <utils/Systrace.h>

using namespace utils;

namespace filament {

using namespace math;

namespace backend {

DriverBase::DriverBase(Dispatcher* dispatcher) noexcept
        : mDispatcher(dispatcher) {
}

DriverBase::~DriverBase() noexcept {
    delete mDispatcher;
}

void DriverBase::purge() noexcept {
    std::vector<BufferDescriptor> buffersToPurge;
    std::vector<AcquiredImage> imagesToPurge;
    std::unique_lock<std::mutex> lock(mPurgeLock);
    std::swap(buffersToPurge, mBufferToPurge);
    std::swap(imagesToPurge, mImagesToPurge);
    lock.unlock(); // don't remove this, it ensures mBufferToPurge is destroyed without lock held
    for (auto& image : imagesToPurge) {
        image.callback(image.image, image.userData);
    }
    // When the BufferDescriptors go out of scope, their destructors invoke their callbacks.
}

void DriverBase::scheduleDestroySlow(BufferDescriptor&& buffer) noexcept {
    std::lock_guard<std::mutex> lock(mPurgeLock);
    mBufferToPurge.push_back(std::move(buffer));
}

// This is called from an async driver method so it's in the GL thread, but purge is called
// on the user thread. This is typically called 0 or 1 times per frame.
void DriverBase::scheduleRelease(AcquiredImage&& image) noexcept {
    std::lock_guard<std::mutex> lock(mPurgeLock);
    mImagesToPurge.push_back(std::move(image));
}

void DriverBase::debugCommandBegin(CommandStream* cmds, bool synchronous, const char* methodName) noexcept {
    if constexpr (bool(FILAMENT_DEBUG_COMMANDS > FILAMENT_DEBUG_COMMANDS_NONE)) {
        if constexpr (bool(FILAMENT_DEBUG_COMMANDS & FILAMENT_DEBUG_COMMANDS_LOG)) {
            utils::slog.d << methodName << utils::io::endl;
        }
        if constexpr (bool(FILAMENT_DEBUG_COMMANDS & FILAMENT_DEBUG_COMMANDS_SYSTRACE)) {
            SYSTRACE_CONTEXT();
            SYSTRACE_NAME_BEGIN(methodName);

            if (!synchronous) {
                cmds->queueCommand([=]() {
                    SYSTRACE_CONTEXT();
                    SYSTRACE_NAME_BEGIN(methodName);
                });
            }
        }
    }
}

void DriverBase::debugCommandEnd(CommandStream* cmds, bool synchronous, const char* methodName) noexcept {
    if constexpr (bool(FILAMENT_DEBUG_COMMANDS > FILAMENT_DEBUG_COMMANDS_NONE)) {
        if constexpr (bool(FILAMENT_DEBUG_COMMANDS & FILAMENT_DEBUG_COMMANDS_SYSTRACE)) {
            if (!synchronous) {
                cmds->queueCommand([]() {
                    SYSTRACE_CONTEXT();
                    SYSTRACE_NAME_END();
                });
            }
            SYSTRACE_CONTEXT();
            SYSTRACE_NAME_END();
        }
    }
}

// ------------------------------------------------------------------------------------------------

Driver::~Driver() noexcept = default;

void Driver::execute(std::function<void(void)> fn) noexcept {
    fn();
}

size_t Driver::getElementTypeSize(ElementType type) noexcept {
    switch (type) {
        case ElementType::BYTE:     return sizeof(int8_t);
        case ElementType::BYTE2:    return sizeof(byte2);
        case ElementType::BYTE3:    return sizeof(byte3);
        case ElementType::BYTE4:    return sizeof(byte4);
        case ElementType::UBYTE:    return sizeof(uint8_t);
        case ElementType::UBYTE2:   return sizeof(ubyte2);
        case ElementType::UBYTE3:   return sizeof(ubyte3);
        case ElementType::UBYTE4:   return sizeof(ubyte4);
        case ElementType::SHORT:    return sizeof(int16_t);
        case ElementType::SHORT2:   return sizeof(short2);
        case ElementType::SHORT3:   return sizeof(short3);
        case ElementType::SHORT4:   return sizeof(short4);
        case ElementType::USHORT:   return sizeof(uint16_t);
        case ElementType::USHORT2:  return sizeof(ushort2);
        case ElementType::USHORT3:  return sizeof(ushort3);
        case ElementType::USHORT4:  return sizeof(ushort4);
        case ElementType::INT:      return sizeof(int32_t);
        case ElementType::UINT:     return sizeof(uint32_t);
        case ElementType::FLOAT:    return sizeof(float);
        case ElementType::FLOAT2:   return sizeof(float2);
        case ElementType::FLOAT3:   return sizeof(float3);
        case ElementType::FLOAT4:   return sizeof(float4);
        case ElementType::HALF:     return sizeof(half);
        case ElementType::HALF2:    return sizeof(half2);
        case ElementType::HALF3:    return sizeof(half3);
        case ElementType::HALF4:    return sizeof(half4);
    }
}

} // namespace backend
} // namespace filament
