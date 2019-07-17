/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2007 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MainWindow.h"
#include "data/osc/OSCQueue.h"

#include "layer/WaveformLayer.h"
#include "view/ViewManager.h"
#include "view/Pane.h"
#include "view/PaneStack.h"
#include "data/model/WaveFileModel.h"
#include "widgets/CommandHistory.h"
#include "audio/AudioCallbackPlaySource.h"
#include "framework/Document.h"
#include "data/fileio/WavFileWriter.h"
#include "transform/TransformFactory.h"
#include "widgets/LevelPanWidget.h"
#include "widgets/LevelPanToolButton.h"
#include "widgets/AudioDial.h"

#include <bqaudioio/SystemPlaybackTarget.h>

#include <QFileInfo>

void
MainWindow::handleOSCMessage(const OSCMessage &message)
{
    SVDEBUG << "OSCHandler: method = \""
            << message.getMethod() << "\"" << endl;

    if (message.getMethod() == "open") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {
            QString path = message.getArg(0).toString();
            if (open(path, ReplaceMainModel) != FileOpenSucceeded) {
                cerr << "OSCHandler: File open failed for path \""
                          << path << "\"" << endl;
            }
            //!!! we really need to spin here and not return until the
            // file has been completely decoded...
        }

    } else if (message.getMethod() == "openadditional") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {
            QString path = message.getArg(0).toString();
            if (open(path, CreateAdditionalModel) != FileOpenSucceeded) {
                cerr << "OSCHandler: File open failed for path \""
                          << path << "\"" << endl;
            }
        }

    } else if (message.getMethod() == "recent" ||
               message.getMethod() == "last") {

        int n = 0;
        if (message.getMethod() == "recent" &&
            message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::Int)) {
            n = message.getArg(0).toInt() - 1;
        }
        std::vector<QString> recent = m_recentFiles.getRecent();
        if (n >= 0 && n < int(recent.size())) {
            if (open(recent[n], ReplaceMainModel) != FileOpenSucceeded) {
                cerr << "OSCHandler: File open failed for path \""
                          << recent[n] << "\"" << endl;
            }
        }

    } else if (message.getMethod() == "save") {

        QString path;
        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {
            path = message.getArg(0).toString();
            if (QFileInfo(path).exists()) {
                SVDEBUG << "OSCHandler: Refusing to overwrite existing file in save" << endl;
            } else {
                saveSessionFile(path);
            }
        }

    } else if (message.getMethod() == "export") {

        QString path;
        if (getMainModel()) {
            if (message.getArgCount() == 1 &&
                message.getArg(0).canConvert(QVariant::String)) {
                path = message.getArg(0).toString();
                if (QFileInfo(path).exists()) {
                    SVDEBUG << "OSCHandler: Refusing to overwrite existing file in export" << endl;
                } else {
                    WavFileWriter writer(path,
                                         getMainModel()->getSampleRate(),
                                         getMainModel()->getChannelCount(),
                                         WavFileWriter::WriteToTemporary);
                    MultiSelection ms = m_viewManager->getSelection();
                    if (!ms.getSelections().empty()) {
                        //!!! todo: update WavFileWriter!
                        writer.writeModel(getMainModel().get(), &ms);
                    } else {
                        writer.writeModel(getMainModel().get());
                    }
                }
            }
        }

    } else if (message.getMethod() == "exportlayer") {

        QString path;
        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {
            path = message.getArg(0).toString();
            if (QFileInfo(path).exists()) {
                SVDEBUG << "OSCHandler: Refusing to overwrite existing file in layer export" << endl;
            } else {
                Pane *currentPane = nullptr;
                Layer *currentLayer = nullptr;
                if (m_paneStack) currentPane = m_paneStack->getCurrentPane();
                if (currentPane) currentLayer = currentPane->getSelectedLayer();
                if (currentLayer) {
                    QString error;
                    if (!exportLayerTo(currentLayer, path, error)) {
                        SVCERR << "OSCHandler: Failed to export current layer to " << path << ": " << error << endl;
                    }
                } else {
                    SVCERR << "OSCHandler: No current layer to export" << endl;
                }
            }
        }

    } else if (message.getMethod() == "jump" ||
               message.getMethod() == "play") {

        if (getMainModel()) {

            sv_frame_t frame = m_viewManager->getPlaybackFrame();
            bool selection = false;
            bool play = (message.getMethod() == "play");

            if (message.getArgCount() == 1) {

                if (message.getArg(0).canConvert(QVariant::String) &&
                    message.getArg(0).toString() == "selection") {

                    selection = true;

                } else if (message.getArg(0).canConvert(QVariant::String) &&
                           message.getArg(0).toString() == "end") {

                    frame = getMainModel()->getEndFrame();

                } else if (message.getArg(0).canConvert(QVariant::Double)) {

                    double time = message.getArg(0).toDouble();
                    if (time < 0.0) time = 0.0;

                    frame = lrint(time * getMainModel()->getSampleRate());
                }
            }

            if (frame > getMainModel()->getEndFrame()) {
                frame = getMainModel()->getEndFrame();
            }

            if (play) {
                m_viewManager->setPlaySelectionMode(selection);
            } 

            if (selection) {
                MultiSelection::SelectionList sl = m_viewManager->getSelections();
                if (!sl.empty()) {
                    frame = sl.begin()->getStartFrame();
                }
            }

            SVDEBUG << "OSCHandler: Setting playback frame to " << frame << endl;

            m_viewManager->setPlaybackFrame(frame);

            if (play) {
                if (!m_playSource->isPlaying()) {
                    SVDEBUG << "OSCHandler: Play source is not yet playing, calling play()" << endl;
                    // handles audio device suspend/resume etc, as
                    // well as calling m_playSource->play(frame)
                    MainWindow::play();
                } else {
                    SVDEBUG << "OSCHandler: Play source is already playing, not starting it" << endl;
                }
            } else {
                SVDEBUG << "OSCHandler: Jump only requested, not starting playback" << endl;
            }
        }

    } else if (message.getMethod() == "ffwd") {

        if (message.getArgCount() == 1) {

            if (message.getArg(0).canConvert(QVariant::String) &&
                message.getArg(0).toString() == "similar") {

                ffwdSimilar();
            }
        } else {

            ffwd();
        }

    } else if (message.getMethod() == "rewind") {

        if (message.getArgCount() == 1) {

            if (message.getArg(0).canConvert(QVariant::String) &&
                message.getArg(0).toString() == "similar") {

                rewindSimilar();
            }
        } else {

            rewind();
        }

    } else if (message.getMethod() == "stop") {
            
            if (m_playSource->isPlaying()) {
                // As with play, we want to use the MainWindow
                // function rather than call m_playSource directly
                // because that way the audio driver suspend/resume
                // etc is handled properly
                MainWindow::stop();
            }

    } else if (message.getMethod() == "loop") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {

            QString str = message.getArg(0).toString();
            if (str == "on") {
                m_viewManager->setPlayLoopMode(true);
            } else if (str == "off") {
                m_viewManager->setPlayLoopMode(false);
            }
        }

    } else if (message.getMethod() == "solo") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {

            QString str = message.getArg(0).toString();
            if (str == "on") {
                m_viewManager->setPlaySoloMode(true);
            } else if (str == "off") {
                m_viewManager->setPlaySoloMode(false);
            }
        }

    } else if (message.getMethod() == "select" ||
               message.getMethod() == "addselect") {

        if (getMainModel()) {

            sv_frame_t f0 = getMainModel()->getStartFrame();
            sv_frame_t f1 = getMainModel()->getEndFrame();

            bool done = false;

            if (message.getArgCount() == 2 &&
                message.getArg(0).canConvert(QVariant::Double) &&
                message.getArg(1).canConvert(QVariant::Double)) {
                
                double t0 = message.getArg(0).toDouble();
                double t1 = message.getArg(1).toDouble();
                if (t1 < t0) { double temp = t0; t0 = t1; t1 = temp; }
                if (t0 < 0.0) t0 = 0.0;
                if (t1 < 0.0) t1 = 0.0;

                f0 = lrint(t0 * getMainModel()->getSampleRate());
                f1 = lrint(t1 * getMainModel()->getSampleRate());
                
                Pane *pane = m_paneStack->getCurrentPane();
                Layer *layer = nullptr;
                if (pane) layer = pane->getSelectedLayer();
                if (layer) {
                    int resolution;
                    layer->snapToFeatureFrame(pane, f0, resolution,
                                              Layer::SnapLeft);
                    layer->snapToFeatureFrame(pane, f1, resolution,
                                              Layer::SnapRight);
                }

            } else if (message.getArgCount() == 1 &&
                       message.getArg(0).canConvert(QVariant::String)) {

                QString str = message.getArg(0).toString();
                if (str == "none") {
                    m_viewManager->clearSelections();
                    done = true;
                }
            }

            if (!done) {
                if (message.getMethod() == "select") {
                    m_viewManager->setSelection(Selection(f0, f1));
                } else {
                    m_viewManager->addSelection(Selection(f0, f1));
                }
            }
        }

    } else if (message.getMethod() == "add") {

        if (getMainModel()) {

            if (message.getArgCount() >= 1 &&
                message.getArg(0).canConvert(QVariant::String)) {

                int channel = -1;
                if (message.getArgCount() == 2 &&
                    message.getArg(0).canConvert(QVariant::Int)) {
                    channel = message.getArg(0).toInt();
                    if (channel < -1 ||
                        channel > int(getMainModel()->getChannelCount())) {
                        cerr << "WARNING: OSCHandler: channel "
                                  << channel << " out of range" << endl;
                        channel = -1;
                    }
                }

                QString str = message.getArg(0).toString();
                
                LayerFactory::LayerType type =
                    LayerFactory::getInstance()->getLayerTypeForName(str);

                if (type == LayerFactory::UnknownLayer) {
                    cerr << "WARNING: OSCHandler: unknown layer "
                              << "type " << str << endl;
                } else {

                    LayerConfiguration configuration(type,
                                                     getMainModelId(),
                                                     channel);
                    
                    addPane(configuration,
                            tr("Add %1 Pane")
                            .arg(LayerFactory::getInstance()->
                                 getLayerPresentationName(type)));
                }
            }
        }

    } else if (message.getMethod() == "undo") {

        CommandHistory::getInstance()->undo();

    } else if (message.getMethod() == "redo") {

        CommandHistory::getInstance()->redo();

    } else if (message.getMethod() == "set") {

        if (message.getArgCount() == 2 &&
            message.getArg(0).canConvert(QVariant::String) &&
            message.getArg(1).canConvert(QVariant::Double)) {

            QString property = message.getArg(0).toString();
            float value = (float)message.getArg(1).toDouble();

            if (property == "gain") {
                if (value < 0.0) value = 0.0;
                m_mainLevelPan->setLevel(value);
                if (m_playTarget) m_playTarget->setOutputGain(value);
            } else if (property == "speed") {
                m_playSpeed->setMappedValue(value);
            } else if (property == "speedup") {
                
                // The speedup method existed before the speed method
                // and is a bit weirder.
                //
                // For speed(x), x is a percentage of normal speed, so
                // x=100 means play at the normal speed, x=50 means
                // half speed, x=200 double speed etc.
                //
                // For speedup(x), x was some sort of modifier of
                // percentage thing, so x=0 meant play at the normal
                // speed, x=50 meant play at 150% of normal speed,
                // x=100 meant play at double speed, and x=-100 rather
                // bizarrely meant play at half speed. We handle this
                // now by converting to speed percentage as follows:
                
                double percentage = 100.0;
                if (value > 0.f) {
                    percentage = percentage + value;
                } else {
                    percentage = 10000.0 / (percentage - value);
                }
                SVDEBUG << "OSCHandler: converted speedup(" << value
                        << ") into speed(" << percentage << ")" << endl;
                    
                m_playSpeed->setMappedValue(percentage);
                
            } else if (property == "overlays") {
                if (value < 0.5) {
                    m_viewManager->setOverlayMode(ViewManager::NoOverlays);
                } else if (value < 1.5) {
                    m_viewManager->setOverlayMode(ViewManager::StandardOverlays);
                } else {
                    m_viewManager->setOverlayMode(ViewManager::AllOverlays);
                }                    
            } else if (property == "zoomwheels") {
                m_viewManager->setZoomWheelsEnabled(value > 0.5);
            } else if (property == "propertyboxes") {
                bool toggle = ((value < 0.5) !=
                               (m_paneStack->getLayoutStyle() == PaneStack::NoPropertyStacks));
                if (toggle) togglePropertyBoxes();
            }
                
        } else {
            PropertyContainer *container = nullptr;
            Pane *pane = m_paneStack->getCurrentPane();
            if (pane &&
                message.getArgCount() == 3 &&
                message.getArg(0).canConvert(QVariant::String) &&
                message.getArg(1).canConvert(QVariant::String) &&
                message.getArg(2).canConvert(QVariant::String)) {
                if (message.getArg(0).toString() == "pane") {
                    container = pane->getPropertyContainer(0);
                } else if (message.getArg(0).toString() == "layer") {
                    container = pane->getSelectedLayer();
                }
            }
            if (container) {
                QString nameString = message.getArg(1).toString();
                QString valueString = message.getArg(2).toString();
                Command *c = container->getSetPropertyCommand
                    (nameString, valueString);
                if (c) CommandHistory::getInstance()->addCommand(c, true, true);
            }
        }

    } else if (message.getMethod() == "setcurrent") {

        int paneIndex = -1, layerIndex = -1;
        bool wantLayer = false;

        if (message.getArgCount() >= 1 &&
            message.getArg(0).canConvert(QVariant::Int)) {

            paneIndex = message.getArg(0).toInt() - 1;

            if (message.getArgCount() >= 2 &&
                message.getArg(1).canConvert(QVariant::Int)) {
                wantLayer = true;
                layerIndex = message.getArg(1).toInt() - 1;
            }
        }

        if (paneIndex >= 0 && paneIndex < m_paneStack->getPaneCount()) {
            Pane *pane = m_paneStack->getPane(paneIndex);
            m_paneStack->setCurrentPane(pane);
            if (layerIndex >= 0 && layerIndex < pane->getLayerCount()) {
                Layer *layer = pane->getLayer(layerIndex);
                m_paneStack->setCurrentLayer(pane, layer);
            } else if (wantLayer && layerIndex == -1) {
                m_paneStack->setCurrentLayer(pane, nullptr);
            }
        }

    } else if (message.getMethod() == "delete") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {
            
            QString target = message.getArg(0).toString();

            if (target == "pane") {

                deleteCurrentPane();

            } else if (target == "layer") {

                deleteCurrentLayer();

            } else {
                
                cerr << "WARNING: OSCHandler: Unknown delete target " << target << endl;
            }
        }

    } else if (message.getMethod() == "zoom") {

        if (message.getArgCount() == 1) {
            if (message.getArg(0).canConvert(QVariant::String) &&
                message.getArg(0).toString() == "in") {
                zoomIn();
            } else if (message.getArg(0).canConvert(QVariant::String) &&
                       message.getArg(0).toString() == "out") {
                zoomOut();
            } else if (message.getArg(0).canConvert(QVariant::String) &&
                       message.getArg(0).toString() == "default") {
                zoomDefault();
            } else if (message.getArg(0).canConvert(QVariant::String) &&
                       message.getArg(0).toString() == "fit") {
                zoomToFit();
            } else if (message.getArg(0).canConvert(QVariant::Double)) {
                double level = message.getArg(0).toDouble();
                Pane *currentPane = m_paneStack->getCurrentPane();
                ZoomLevel zoomLevel;
                if (level >= 0.66) {
                    zoomLevel = ZoomLevel(ZoomLevel::FramesPerPixel,
                                          int(round(level)));
                } else {
                    zoomLevel = ZoomLevel(ZoomLevel::PixelsPerFrame,
                                          int(round(1.0 / level)));
                }
                if (currentPane) {
                    currentPane->setZoomLevel(zoomLevel);
                }
            }
        }

    } else if (message.getMethod() == "zoomvertical") {

        Pane *pane = m_paneStack->getCurrentPane();
        Layer *layer = nullptr;
        if (pane && pane->getLayerCount() > 0) {
            layer = pane->getLayer(pane->getLayerCount() - 1);
        }
        int defaultStep = 0;
        int steps = 0;
        if (layer) {
            steps = layer->getVerticalZoomSteps(defaultStep);
            if (message.getArgCount() == 1 && steps > 0) {
                if (message.getArg(0).canConvert(QVariant::String) &&
                    message.getArg(0).toString() == "in") {
                    int step = layer->getCurrentVerticalZoomStep() + 1;
                    if (step < steps) layer->setVerticalZoomStep(step);
                } else if (message.getArg(0).canConvert(QVariant::String) &&
                           message.getArg(0).toString() == "out") {
                    int step = layer->getCurrentVerticalZoomStep() - 1;
                    if (step >= 0) layer->setVerticalZoomStep(step);
                } else if (message.getArg(0).canConvert(QVariant::String) &&
                           message.getArg(0).toString() == "default") {
                    layer->setVerticalZoomStep(defaultStep);
                }
            } else if (message.getArgCount() == 2) {
                if (message.getArg(0).canConvert(QVariant::Double) &&
                    message.getArg(1).canConvert(QVariant::Double)) {
                    double min = message.getArg(0).toDouble();
                    double max = message.getArg(1).toDouble();
                    layer->setDisplayExtents(min, max);
                }
            }
        }

    } else if (message.getMethod() == "quit") {
        
        m_abandoning = true;
        close();

    } else if (message.getMethod() == "resize") {
        
        if (message.getArgCount() == 2) {

            int width = 0, height = 0;

            if (message.getArg(1).canConvert(QVariant::Int)) {

                height = message.getArg(1).toInt();

                if (message.getArg(0).canConvert(QVariant::String) &&
                    message.getArg(0).toString() == "pane") {

                    Pane *pane = m_paneStack->getCurrentPane();
                    if (pane) pane->resize(pane->width(), height);

                } else if (message.getArg(0).canConvert(QVariant::Int)) {

                    width = message.getArg(0).toInt();
                    resize(width, height);
                }
            }
        }

    } else if (message.getMethod() == "transform") {

        Pane *pane = m_paneStack->getCurrentPane();

        if (getMainModel() &&
            pane &&
            message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {

            TransformId transformId = message.getArg(0).toString();

            Transform transform = TransformFactory::getInstance()->
                getDefaultTransformFor(transformId);
            
            Layer *newLayer = m_document->createDerivedLayer
                (transform, getMainModelId());

            if (newLayer) {
                m_document->addLayerToView(pane, newLayer);
                m_recentTransforms.add(transformId);
                m_paneStack->setCurrentLayer(pane, newLayer);
            }
        }

    } else {
        cerr << "WARNING: OSCHandler: Unknown or unsupported "
                  << "method \"" << message.getMethod()
                  << "\"" << endl;
    }
}
