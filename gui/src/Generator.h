#pragma once
#include "AsyncTask.h"
#include "Define.h"
#include <string>

enum class GeneratorShapes : std::uint32_t {
    Rectangle        = 1,
    RotatedRectangle = 2,
    Ellipse          = 8,
    RotatedEllipse   = 16,
    Circle           = 32,
};

struct ShapeGeneratorOptions {
    int mutationCount      = 100;                             // Mutations per candidate
    int candidateCount     = 100;                             // Candidates per step
    int maxShapeCount      = 128;                             // Max number of shapes to generate
    int shapeAlpha         = 255;                             // Alpha component of shape color
    GeneratorShapes shapes = GeneratorShapes::RotatedEllipse; // Allowed shape types
};

class ShapeGenerator {
    MAKE_NONCOPYABLE(ShapeGenerator);

public:
    class ShapeGeneratorImpl;

    ShapeGenerator(int width, int height, const std::vector<uint8_t>& pixelData, const ShapeGeneratorOptions& options);
    ~ShapeGenerator();

    void run();
    void cancel();
    bool done();
    bool hasNewState();
    std::string getJson();

private:
    std::unique_ptr<ShapeGeneratorImpl> impl;
};
