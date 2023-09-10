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
    int mutationCount  = 50;  // Mutations per candidate
    int candidateCount = 50;  // Candidates per step
    int maxShapeCount  = 128; // Max number of shapes to generate
    int shapeAlpha     = 128; // Alpha component of shape color
    GeneratorShapes shapes;   // Allowed shape types
};

class ShapeGenerator {
    MAKE_NONCOPYABLE(ShapeGenerator);

public:
    class ShapeGeneratorImpl;

    ShapeGenerator(int width, int height, const std::vector<uint8_t>& pixelData, const ShapeGeneratorOptions& options);
    ~ShapeGenerator();

    void run();
    bool done();
    bool hasNewState();
    std::string getJson();

private:
    std::unique_ptr<ShapeGeneratorImpl> impl;
};
