#pragma once

#include <juce_core/juce_core.h>
#include <mysofa.h>

// ==============================================================================
// SofaLoader — C++ RAII wrapper around libmysofa's MYSOFA_EASY API.
//
// Usage:
//   SofaLoader loader;
//   if (loader.load("/path/to/hrtf.sofa", 48000.0))
//       loader.getHRTF(azimuthDeg, elevationDeg, irLeft, irRight, delayL, delayR);
// ==============================================================================
class SofaLoader
{
public:
    SofaLoader() = default;
    ~SofaLoader() { close(); }

    // Non-copyable, movable
    SofaLoader(const SofaLoader&) = delete;
    SofaLoader& operator=(const SofaLoader&) = delete;
    SofaLoader(SofaLoader&& other) noexcept { *this = std::move(other); }
    SofaLoader& operator=(SofaLoader&& other) noexcept
    {
        if (this != &other) { close(); std::swap(easy, other.easy); std::swap(filterLength, other.filterLength); }
        return *this;
    }

    // --------------------------------------------------------------------------
    // Load a SOFA file at the target sample rate. Returns true on success.
    // --------------------------------------------------------------------------
    bool load(const juce::String& filePath, double sampleRate)
    {
        close();

        int error = 0;
        easy = mysofa_open(filePath.toRawUTF8(), static_cast<float>(sampleRate),
                           &filterLength, &error);

        if (easy == nullptr)
        {
            juce::Logger::writeToLog("SofaLoader: failed to open " + filePath
                                     + " — error " + juce::String(error));
            return false;
        }

        juce::Logger::writeToLog("SofaLoader: loaded " + filePath
                                 + " — filter length " + juce::String(filterLength)
                                 + " samples");
        return true;
    }

    // --------------------------------------------------------------------------
    // Load SOFA from raw data (e.g., embedded binary resource). Returns true on success.
    // --------------------------------------------------------------------------
    bool loadFromMemory(const char* data, size_t size, double sampleRate)
    {
        close();

        int error = 0;
        easy = mysofa_open_data(data, static_cast<long>(size),
                                static_cast<float>(sampleRate),
                                &filterLength, &error);

        if (easy == nullptr)
        {
            juce::Logger::writeToLog("SofaLoader: failed to load from memory — error "
                                     + juce::String(error));
            return false;
        }

        juce::Logger::writeToLog("SofaLoader: loaded from memory — filter length "
                                 + juce::String(filterLength) + " samples");
        return true;
    }

    // --------------------------------------------------------------------------
    // Retrieve HRTF impulse response for a source at the given direction.
    //
    // azimuthDeg  — horizontal angle in degrees (0° = front, 90° = right)
    // elevationDeg — vertical angle in degrees (0° = horizon, + = up)
    // irLeft/irRight  — pre-allocated float arrays (size = filterLength)
    // delayLeft/delayRight — output delays in samples
    //
    // libmysofa convention:
    //   x = right      (positive = right)
    //   y = front      (positive = front)
    //   z = up         (positive = up)
    // --------------------------------------------------------------------------
    void getHRTF(double azimuthDeg, double elevationDeg,
                 float* irLeft, float* irRight,
                 float& delayLeft, float& delayRight) const
    {
        if (easy == nullptr) return;

        // Convert spherical to Cartesian; libmysofa expects unit-length vector.
        float azRad = static_cast<float>(juce::degreesToRadians(azimuthDeg));
        float elRad = static_cast<float>(juce::degreesToRadians(elevationDeg));

        float cosEl = std::cos(elRad);
        float x = std::sin(azRad) * cosEl;   // right
        float y = std::cos(azRad) * cosEl;   // front
        float z = std::sin(elRad);           // up

        mysofa_getfilter_float(easy, x, y, z, irLeft, irRight,
                               &delayLeft, &delayRight);
    }

    int getFilterLength() const noexcept { return filterLength; }
    bool isLoaded() const noexcept { return easy != nullptr; }

    void close()
    {
        if (easy != nullptr) { mysofa_close(easy); easy = nullptr; }
        filterLength = 0;
    }

private:
    MYSOFA_EASY* easy = nullptr;
    int          filterLength = 0;
};
