#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstdlib>
#include <iostream>

#include "decoder/keycode_decoder.hpp"
#include "model/keymap_model.hpp"

#define TEST_ASSERT(cond)                                                                          \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            std::cerr << "ASSERTION FAILED: " #cond << " at " << __FILE__ << ":" << __LINE__       \
                      << "\n";                                                                     \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "Running layout model tests...\n";

    // 1. Verify Eyelash Corne layout profile directly
    {
        QFile file("resources/layouts/eyelash_corne.json");
        if (!file.open(QIODevice::ReadOnly)) {
            file.setFileName("../resources/layouts/eyelash_corne.json");
            TEST_ASSERT(file.open(QIODevice::ReadOnly));
        }
        auto doc = QJsonDocument::fromJson(file.readAll());
        TEST_ASSERT(doc.isObject());
        auto obj = doc.object();

        TEST_ASSERT(obj.value("id").toString() == "eyelash_corne");
        TEST_ASSERT(obj.value("totalKeys").toInt() == 48);

        auto leftHalf = obj.value("leftHalf").toObject();
        auto leftCols = leftHalf.value("columns").toArray();
        TEST_ASSERT(leftCols.size() == 6);
        TEST_ASSERT(leftHalf.value("thumbCluster").toArray().size() == 3);

        auto rightHalf = obj.value("rightHalf").toObject();
        auto rightCols = rightHalf.value("columns").toArray();
        TEST_ASSERT(rightCols.size() == 6);
        TEST_ASSERT(rightHalf.value("thumbCluster").toArray().size() == 3);

        auto center = obj.value("centerControls").toArray();
        TEST_ASSERT(center.size() == 2);
        TEST_ASSERT(center.at(0).toObject().value("type").toString() == "rotary_knob");
        TEST_ASSERT(center.at(1).toObject().value("type").toString() == "joystick_5way");

        std::cout << "[PASS] Eyelash Corne layout profile verified\n";
    }

    // 2. Verify ZSA Voyager layout profile directly
    {
        QFile file("resources/layouts/voyager.json");
        if (!file.open(QIODevice::ReadOnly)) {
            file.setFileName("../resources/layouts/voyager.json");
            TEST_ASSERT(file.open(QIODevice::ReadOnly));
        }
        auto doc = QJsonDocument::fromJson(file.readAll());
        TEST_ASSERT(doc.isObject());
        auto obj = doc.object();

        TEST_ASSERT(obj.value("id").toString() == "voyager");
        TEST_ASSERT(obj.value("totalKeys").toInt() == 52);

        auto leftHalf = obj.value("leftHalf").toObject();
        auto leftCols = leftHalf.value("columns").toArray();
        TEST_ASSERT(leftCols.size() == 6);
        // Each column has 4 keys
        for (const auto &colVal : leftCols) {
            TEST_ASSERT(colVal.toObject().value("keys").toArray().size() == 4);
        }
        TEST_ASSERT(leftHalf.value("thumbCluster").toArray().size() == 2);

        auto rightHalf = obj.value("rightHalf").toObject();
        auto rightCols = rightHalf.value("columns").toArray();
        TEST_ASSERT(rightCols.size() == 6);
        for (const auto &colVal : rightCols) {
            TEST_ASSERT(colVal.toObject().value("keys").toArray().size() == 4);
        }
        TEST_ASSERT(rightHalf.value("thumbCluster").toArray().size() == 2);

        auto center = obj.value("centerControls").toArray();
        TEST_ASSERT(center.isEmpty()); // Voyager has no rotary/joystick in center

        std::cout << "[PASS] ZSA Voyager layout profile verified\n";
    }

    // 3. Verify TwinDial 25 layout profile directly
    {
        QFile file("resources/layouts/twindial25.json");
        if (!file.open(QIODevice::ReadOnly)) {
            file.setFileName("../resources/layouts/twindial25.json");
            TEST_ASSERT(file.open(QIODevice::ReadOnly));
        }
        auto doc = QJsonDocument::fromJson(file.readAll());
        TEST_ASSERT(doc.isObject());
        auto obj = doc.object();

        TEST_ASSERT(obj.value("id").toString() == "twindial25");
        TEST_ASSERT(obj.value("type").toString() == "macropad");
        TEST_ASSERT(obj.value("totalKeys").toInt() == 24);

        auto grid = obj.value("grid").toObject();
        auto rowsData = grid.value("rowsData").toArray();
        TEST_ASSERT(rowsData.size() == 5);

        std::cout << "[PASS] TwinDial 25 layout profile verified\n";
    }

    // 4. Test KeymapModel loading layouts and declarative sensor defaults
    {
        strata::model::KeymapModel model(nullptr);
        bool loadedCorne = model.loadLayout("eyelash_corne");
        TEST_ASSERT(loadedCorne);
        TEST_ASSERT(model.layoutId() == "eyelash_corne");
        TEST_ASSERT(model.totalKeys() == 48);

        // Check sensor default on layer 0 (Volume control)
        model.setCurrentLayer(0);
        auto sData0 = model.getSensorData(0);
        TEST_ASSERT(sData0.value("hasData").toBool());
        TEST_ASSERT(sData0.value("cwLabel").toString() == "VOL+");
        TEST_ASSERT(sData0.value("ccwLabel").toString() == "VOL-");
        TEST_ASSERT(sData0.value("pressKeyPos").toInt() == 34);

        // Check sensor default on layer 1 (Mouse scroll)
        model.setCurrentLayer(1);
        auto sData1 = model.getSensorData(0);
        TEST_ASSERT(sData1.value("hasData").toBool());
        TEST_ASSERT(sData1.value("cwLabel").toString() == "SCRL DN");
        TEST_ASSERT(sData1.value("ccwLabel").toString() == "SCRL UP");

        // Switch to Voyager layout
        bool loadedVoyager = model.loadLayout("voyager");
        TEST_ASSERT(loadedVoyager);
        TEST_ASSERT(model.layoutId() == "voyager");
        TEST_ASSERT(model.totalKeys() == 52);

        // Switch to TwinDial 25 layout
        bool loadedTwinDial = model.loadLayout("twindial25");
        TEST_ASSERT(loadedTwinDial);
        TEST_ASSERT(model.layoutId() == "twindial25");
        TEST_ASSERT(model.totalKeys() == 24);

        // Check sensor defaults on TwinDial layer 0 (Zoom and Volume)
        model.setCurrentLayer(0);
        auto tdS0 = model.getSensorData(0);
        TEST_ASSERT(tdS0.value("hasData").toBool());
        TEST_ASSERT(tdS0.value("cwLabel").toString() == "ZOOM-");
        TEST_ASSERT(tdS0.value("ccwLabel").toString() == "ZOOM+");

        auto tdS1 = model.getSensorData(1);
        TEST_ASSERT(tdS1.value("hasData").toBool());
        TEST_ASSERT(tdS1.value("cwLabel").toString() == "VOL-");
        TEST_ASSERT(tdS1.value("ccwLabel").toString() == "VOL+");

        std::cout << "[PASS] KeymapModel declarative layout switching and sensor defaults\n";
    }

    std::cout << "All layout model tests passed successfully!\n";
    return 0;
}
