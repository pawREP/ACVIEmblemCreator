#include "Generator.h"
#include "Define.h"
#include "geometrize/bitmap/bitmap.h"
#include "geometrize/commonutil.h"
#include "geometrize/exporter/shapejsonexporter.h"
#include "geometrize/runner/imagerunner.h"
#include "geometrize/runner/imagerunneroptions.h"
#include "geometrize/shape/rectangle.h"
#include "geometrize/shape/shapefactory.h"
#include "json.h"
#include <mutex>

class ShapeGenerator::ShapeGeneratorImpl {
    MAKE_NONCOPYABLE(ShapeGeneratorImpl);

public:
    ShapeGeneratorImpl(const std::vector<uint8_t>& pixelData) : bitmap{ 256, 256, pixelData } { // TODO: w/h
    }

    void run() {
        auto proc = [this]() {
            geometrize::ImageRunnerOptions options;
            options.alpha             = 255;
            options.maxShapeMutations = 100;
            options.shapeCount        = 100; // candidates
            options.shapeTypes        = { geometrize::ShapeTypes::ROTATED_ELLIPSE };

            geometrize::ImageRunner runner{ bitmap };

            std::vector<geometrize::ShapeResult> shapeData;

            const auto shape            = geometrize::create(geometrize::ShapeTypes::RECTANGLE);
            geometrize::Rectangle* rect = dynamic_cast<geometrize::Rectangle*>(shape.get());
            rect->m_x1                  = 0;
            rect->m_y1                  = 0;
            rect->m_x2                  = static_cast<float>(runner.getCurrent().getWidth());
            rect->m_y2                  = static_cast<float>(runner.getCurrent().getHeight());
            shapeData.emplace_back(geometrize::ShapeResult{ 0, geometrize::commonutil::getAverageImageColor(bitmap), shape });

            for(std::size_t steps = 0; steps < 127; steps++) {
                const std::vector<geometrize::ShapeResult> shapeResults{ runner.step(options) };
                assert(shapeResults.size() == 1); // More than one shape per step possible??

                std::copy(shapeResults.begin(), shapeResults.end(), std::back_inserter(shapeData));
                auto jsonStringTmp = geometrize::exporter::exportShapeJson(shapeData);

                std::lock_guard lock{ jsonMutex };
                jsonString = std::move(jsonStringTmp);
                hasNewData = true;
            }
        };

        runner = { proc };
        runner.run();
    }

    bool done() {
        return runner.ready();
    }

    bool hasNewState() {
        return hasNewData;
    }

    std::string getJson() {
        std::lock_guard lock{ jsonMutex };
        hasNewData = false;
        auto ret   = jsonString;
        return ret;
    }

private:
    std::mutex jsonMutex;
    std::string jsonString;
    bool hasNewData = false;

    geometrize::Bitmap bitmap;
    AsyncTask<void()> runner;
};

ShapeGenerator::ShapeGenerator(const std::vector<uint8_t>& pixelData)
: impl(std::make_unique<ShapeGeneratorImpl>(pixelData)) {
}

ShapeGenerator::~ShapeGenerator() = default;

void ShapeGenerator::run() {
    impl->run();
}

bool ShapeGenerator::done() {
    return impl->done();
}

bool ShapeGenerator::hasNewState() {
    return impl->hasNewState();
}

std::string ShapeGenerator::getJson() {
    return impl->getJson();
}
