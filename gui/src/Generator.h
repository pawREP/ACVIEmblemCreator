#pragma once
#include "AsyncTask.h"
#include "Define.h"
#include <string>

class ShapeGenerator {
    MAKE_NONCOPYABLE(ShapeGenerator);

public:
    class ShapeGeneratorImpl;

    ShapeGenerator(const std::vector<uint8_t>& pixelData);
    ~ShapeGenerator();

    void run();
    bool done();
    bool hasNewState();
    std::string getJson();

private:
    std::unique_ptr<ShapeGeneratorImpl> impl;
};
