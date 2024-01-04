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
#include <QTime>
#include <QElapsedTimer>

#if (QT_VERSION >= 0x050600)
#define NOW (QTime::currentTime().toString(Qt::ISODateWithMs))
#else
#define NOW (QTime::currentTime().toString(Qt::ISODate))
#endif

void
MainWindow::handleOSCMessage(const OSCMessage &message)
{
    QElapsedTimer timer;
    timer.start();
    
    SVDEBUG << "OSCHandler at " << NOW << ": handling message: "
            << message.toString() << endl;

    if (message.getMethod() == "open") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {
            QString path = message.getArg(0).toString();
            if (open(path, ReplaceMainModel) == FileOpenSucceeded) {
                SVDEBUG << "OSCHandler: Opened path \""
                        << path << "\"" << endl;
            } else {
                SVCERR << "OSCHandler: File open failed for path \""
                       << path << "\"" << endl;
            }
            //!!! we really need to spin here and not return until the
            // file has been completely decoded...
        } else {
            SVCERR << "OSCHandler: Usage: /open <filename>" << endl;
        }

    } else if (message.getMethod() == "openadditional") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {
            QString path = message.getArg(0).toString();
            if (open(path, CreateAdditionalModel) == FileOpenSucceeded) {
                SVDEBUG << "OSCHandler: Opened additional path \""
                        << path << "\"" << endl;
            } else {
                SVCERR << "OSCHandler: File open failed for path \""
                          << path << "\"" << endl;
            }
        } else {
            SVCERR << "OSCHandler: Usage: /openadditional <filename>" << endl;
        }

    } else if (message.getMethod() == "recent" ||
               message.getMethod() == "last") {

        int n = 0;
        if (message.getMethod() == "recent" &&
            message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::Int))) {
            n = message.getArg(0).toInt() - 1;
        }
        std::vector<QString> recent = m_recentFiles.getRecent();
        if (n >= 0 && n < int(recent.size())) {
            QString path = recent[n];
            if (open(path, ReplaceMainModel) == FileOpenSucceeded) {
                SVDEBUG << "OSCHandler: Opened recent path \""
                        << path << "\"" << endl;
            } else {
                SVCERR << "OSCHandler: File open failed for path \""
                       << path << "\"" << endl;
            }
        } else {
            SVCERR << "OSCHandler: Usage: /recent <n>" << endl;
            SVCERR << "               or  /last" << endl;
        }

    } else if (message.getMethod() == "save") {

        QString path;
        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {
            path = message.getArg(0).toString();
            if (QFileInfo(path).exists()) {
                SVCERR << "OSCHandler: Refusing to overwrite existing file in save" << endl;
            } else if (saveSessionFile(path)) {
                SVDEBUG << "OSCHandler: Saved session to path \""
                        << path << "\"" << endl;
            } else {
                SVCERR << "OSCHandler: Save failed to path \""
                       << path << "\"" << endl;
            }
        } else {
            SVCERR << "OSCHandler: Usage: /save <filename>" << endl;
        }

    } else if (message.getMethod() == "export") {

        QString path;
        if (getMainModel()) {
            if (message.getArgCount() == 1 &&
                message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {
                path = message.getArg(0).toString();
                if (QFileInfo(path).exists()) {
                    SVCERR << "OSCHandler: Refusing to overwrite existing file in export" << endl;
                } else {
                    WavFileWriter writer(path,
                                         getMainModel()->getSampleRate(),
                                         getMainModel()->getChannelCount(),
                                         WavFileWriter::WriteToTemporary);
                    MultiSelection ms = m_viewManager->getSelection();
                    if (writer.writeModel
                        (getMainModel().get(),
                         ms.getSelections().empty() ? nullptr : &ms)) {
                        SVDEBUG << "OSCHandler: Exported audio to path \""
                                << path << "\"" << endl;
                    } else {
                        SVCERR << "OSCHandler: Export failed to path \""
                               << path << "\"" << endl;
                    }
                }
            }
        } else {
            SVCERR << "OSCHandler: Usage: /export <filename>" << endl;
        }

    } else if (message.getMethod() == "exportlayer") {

        QString path;
        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {
            path = message.getArg(0).toString();
            if (QFileInfo(path).exists()) {
                SVCERR << "OSCHandler: Refusing to overwrite existing file in layer export" << endl;
            } else {
                Pane *currentPane = nullptr;
                Layer *currentLayer = nullptr;
                if (m_paneStack) currentPane = m_paneStack->getCurrentPane();
                if (currentPane) currentLayer = currentPane->getSelectedLayer();
                MultiSelection ms = m_viewManager->getSelection();
                if (currentLayer) {
                    QString error;
                    if (exportLayerTo
                        (currentLayer, currentPane,
                         ms.getSelections().empty() ? nullptr : &ms,
                         path, error)) {
                        SVDEBUG << "OSCHandler: Exported layer \""
                                << currentLayer->getLayerPresentationName()
                                << "\" to path \"" << path << "\"" << endl;
                    } else {
                        SVCERR << "OSCHandler: Export failed to path \""
                               << path << "\"" << endl;
                    }
                } else {
                    SVCERR << "OSCHandler: No current layer to export" << endl;
                }
            }
        } else {
            SVCERR << "OSCHandler: Usage: /exportlayer <filename>" << endl;
        }

    } else if (message.getMethod() == "exportimage") {

        QString path;
        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {
            path = message.getArg(0).toString();
            if (QFileInfo(path).exists()) {
                SVCERR << "OSCHandler: Refusing to overwrite existing file in image export" << endl;
            } else {
                Pane *currentPane = nullptr;
                if (m_paneStack) currentPane = m_paneStack->getCurrentPane();
                MultiSelection ms = m_viewManager->getSelection();
                if (currentPane) {
                    QImage *image = nullptr;
                    auto sel = ms.getSelections();
                    if (!sel.empty()) {
                        sv_frame_t sf0 = sel.begin()->getStartFrame();
                        sv_frame_t sf1 = sel.rbegin()->getEndFrame();
                        image = currentPane->renderPartToNewImage(sf0, sf1);
                    } else {
                        image = currentPane->renderToNewImage();
                    }
                    if (!image) {
                        SVCERR << "OSCHandler: Failed to create image from pane"
                               << endl;
                    } else if (!image->save(path, "PNG")) {
                        SVCERR << "OSCHandler: Export failed to image file \""
                               << path << "\"" << endl;
                    } else {                        
                        SVDEBUG << "OSCHandler: Exported pane to image file \""
                               << path << "\"" << endl;
                    }
                    delete image;
                } else {
                    SVCERR << "OSCHandler: No current pane to export" << endl;
                }
            }
        } else {
            SVCERR << "OSCHandler: Usage: /exportimage <filename>" << endl;
        }

    } else if (message.getMethod() == "exportsvg") {

        QString path;
        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {
            path = message.getArg(0).toString();
            if (QFileInfo(path).exists()) {
                SVCERR << "OSCHandler: Refusing to overwrite existing file in SVG export" << endl;
            } else {
                Pane *currentPane = nullptr;
                if (m_paneStack) currentPane = m_paneStack->getCurrentPane();
                MultiSelection ms = m_viewManager->getSelection();
                if (currentPane) {
                    bool result = false;
                    auto sel = ms.getSelections();
                    if (!sel.empty()) {
                        sv_frame_t sf0 = sel.begin()->getStartFrame();
                        sv_frame_t sf1 = sel.rbegin()->getEndFrame();
                        result = currentPane->renderPartToSvgFile(path, sf0, sf1);
                    } else {
                        result = currentPane->renderToSvgFile(path);
                    }
                    if (!result) {
                        SVCERR << "OSCHandler: Export failed to SVG file \""
                               << path << "\"" << endl;
                    } else {                        
                        SVDEBUG << "OSCHandler: Exported pane to SVG file \""
                                << path << "\"" << endl;
                    }
                } else {
                    SVCERR << "OSCHandler: No current pane to export" << endl;
                }
            }
        } else {
            SVCERR << "OSCHandler: Usage: /exportsvg <filename>" << endl;
        }

    } else if (message.getMethod() == "jump" ||
               message.getMethod() == "play") {

        if (getMainModel()) {

            sv_frame_t frame = m_viewManager->getPlaybackFrame();
            bool selection = false;
            bool play = (message.getMethod() == "play");

            if (message.getArgCount() == 1) {

                if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                    message.getArg(0).toString() == "selection") {

                    selection = true;

                } else if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                           message.getArg(0).toString() == "end") {

                    frame = getMainModel()->getEndFrame();

                } else if (message.getArg(0).canConvert(QMetaType(QMetaType::Double))) {

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

            if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                message.getArg(0).toString() == "similar") {

                SVDEBUG << "OSCHandler: Calling ffwdSimilar" << endl;
                ffwdSimilar();
            }
        } else {

            SVDEBUG << "OSCHandler: Calling ffwd" << endl;
            ffwd();
        }

    } else if (message.getMethod() == "rewind") {

        if (message.getArgCount() == 1) {

            if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                message.getArg(0).toString() == "similar") {

                SVDEBUG << "OSCHandler: Calling rewindSimilar" << endl;
                rewindSimilar();
            }
        } else {

            SVDEBUG << "OSCHandler: Calling rewind" << endl;
            rewind();
        }

    } else if (message.getMethod() == "stop") {
            
        if (m_playSource->isPlaying()) {
            // As with play, we want to use the MainWindow function
            // rather than call m_playSource directly because that way
            // the audio driver suspend/resume etc is handled properly
            SVDEBUG << "OSCHandler: Calling stop" << endl;
            MainWindow::stop();
        } else {
            SVDEBUG << "OSCHandler: Not playing, doing nothing" << endl;
        }

    } else if (message.getMethod() == "loop") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {

            QString str = message.getArg(0).toString();
            if (str == "on") {
                SVDEBUG << "OSCHandler: Enabling loop mode" << endl;
                m_viewManager->setPlayLoopMode(true);
            } else if (str == "off") {
                SVDEBUG << "OSCHandler: Disabling loop mode" << endl;
                m_viewManager->setPlayLoopMode(false);
            }
        }

    } else if (message.getMethod() == "solo") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {

            QString str = message.getArg(0).toString();
            if (str == "on") {
                SVDEBUG << "OSCHandler: Enabling solo mode" << endl;
                m_viewManager->setPlaySoloMode(true);
            } else if (str == "off") {
                SVDEBUG << "OSCHandler: Disabling solo mode" << endl;
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
                message.getArg(0).canConvert(QMetaType(QMetaType::Double)) &&
                message.getArg(1).canConvert(QMetaType(QMetaType::Double))) {
                
                double t0 = message.getArg(0).toDouble();
                double t1 = message.getArg(1).toDouble();
                if (t1 < t0) { double temp = t0; t0 = t1; t1 = temp; }
                if (t0 < 0.0) t0 = 0.0;
                if (t1 < 0.0) t1 = 0.0;

                f0 = lrint(t0 * getMainModel()->getSampleRate());
                f1 = lrint(t1 * getMainModel()->getSampleRate());

                SVDEBUG << "OSCHandler: Converted selection extents to frames "
                        << f0 << " and " << f1 << endl;
                
                Pane *pane = m_paneStack->getCurrentPane();
                Layer *layer = nullptr;
                if (pane) layer = pane->getSelectedLayer();
                if (layer) {
                    int resolution;
                    layer->snapToFeatureFrame(pane, f0, resolution,
                                              Layer::SnapLeft, -1);
                    layer->snapToFeatureFrame(pane, f1, resolution,
                                              Layer::SnapRight, -1);

                    SVDEBUG << "OSCHandler: Snapped selection extents to "
                            << f0 << " and " << f1 << " for current layer \""
                            << layer->getLayerPresentationName() << "\""
                            << endl;
                }

            } else if (message.getArgCount() == 1 &&
                       message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {
                
                QString str = message.getArg(0).toString();
                if (str == "none") {
                    SVDEBUG << "OSCHandler: Clearing selection" << endl;
                    m_viewManager->clearSelections();
                    done = true;
                } else if (str == "all") {
                    SVDEBUG << "OSCHandler: Selecting all" << endl;
                    f0 = getModelsStartFrame();
                    f0 = getModelsEndFrame();
                }
            }

            if (!done) {
                if (message.getMethod() == "select") {
                    m_viewManager->setSelection(Selection(f0, f1));
                } else {
                    m_viewManager->addSelection(Selection(f0, f1));
                }
            }

            SVDEBUG << "OSCHandler: Selection now is "
                    << m_viewManager->getSelection().toString() << endl;

        } else {
            SVCERR << "OSCHandler: No main model, can't modify selection"
                   << endl;
        }

    } else if (message.getMethod() == "add") {

        if (getMainModel()) {

            if (message.getArgCount() >= 1 &&
                message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {

                int channel = -1;
                if (message.getArgCount() == 2 &&
                    message.getArg(0).canConvert(QMetaType(QMetaType::Int))) {
                    channel = message.getArg(0).toInt();
                    if (channel < -1 ||
                        channel >= int(getMainModel()->getChannelCount())) {
                        SVCERR << "OSCHandler: channel " << channel
                               << " out of range (0 to "
                               << (getMainModel()->getChannelCount() - 1)
                               << ")" << endl;
                        channel = -1;
                    }
                }

                QString str = message.getArg(0).toString();
                
                LayerFactory::LayerType type =
                    LayerFactory::getInstance()->getLayerTypeForName(str);

                if (type == LayerFactory::UnknownLayer) {
                    SVCERR << "WARNING: OSCHandler: unknown layer "
                           << "type " << str << endl;
                } else {

                    LayerConfiguration configuration(type,
                                                     getMainModelId(),
                                                     channel);

                    QString pname = LayerFactory::getInstance()->
                        getLayerPresentationName(type);
                    
                    addPane(configuration, tr("Add %1 Pane") .arg(pname));

                    SVDEBUG << "OSCHandler: Added pane \"" << pname
                            << "\"" << endl;
                }
            } else {
                SVCERR << "OSCHandler: Usage: /add <layertype> [<channel>]"
                       << endl;
            }
        }

    } else if (message.getMethod() == "undo") {

        SVDEBUG << "OSCHandler: Calling undo" << endl;
        CommandHistory::getInstance()->undo();

    } else if (message.getMethod() == "redo") {

        SVDEBUG << "OSCHandler: Calling redo" << endl;
        CommandHistory::getInstance()->redo();

    } else if (message.getMethod() == "set") {

        if (message.getArgCount() == 2 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
            message.getArg(1).canConvert(QMetaType(QMetaType::Double))) {

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
                               (m_paneStack->getLayoutStyle() ==
                                PaneStack::HiddenPropertyStacksLayout));
                if (toggle) togglePropertyBoxes();
            }
                
        } else {
            PropertyContainer *container = nullptr;
            Pane *pane = m_paneStack->getCurrentPane();
            if (pane &&
                message.getArgCount() == 3 &&
                message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                message.getArg(1).canConvert(QMetaType(QMetaType::QString)) &&
                message.getArg(2).canConvert(QMetaType(QMetaType::QString))) {
                if (message.getArg(0).toString() == "pane") {
                    container = pane->getPropertyContainer(0);
                } else if (message.getArg(0).toString() == "layer") {
                    container = pane->getSelectedLayer();
                }
            } else {
                SVCERR << "OSCHandler: Usage: /set <control> <value>" << endl
                       << "               or  /set pane <control> <value>" << endl
                       << "               or  /set layer <control> <value>" << endl;
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
            message.getArg(0).canConvert(QMetaType(QMetaType::Int))) {

            paneIndex = message.getArg(0).toInt() - 1;

            if (message.getArgCount() >= 2 &&
                message.getArg(1).canConvert(QMetaType(QMetaType::Int))) {
                wantLayer = true;
                layerIndex = message.getArg(1).toInt() - 1;
            }
        } else {
            SVCERR << "OSCHandler: Usage: /setcurrent <pane> [<layer>]" << endl;
        }

        if (paneIndex >= 0 && paneIndex < m_paneStack->getPaneCount()) {
            Pane *pane = m_paneStack->getPane(paneIndex);
            m_paneStack->setCurrentPane(pane);
            SVDEBUG << "OSCHandler: Set current pane to index "
                    << paneIndex << " (pane id " << pane->getId()
                    << ")" << endl;
            if (layerIndex >= 0 && layerIndex < pane->getLayerCount()) {
                Layer *layer = pane->getFixedOrderLayer(layerIndex);
                m_paneStack->setCurrentLayer(pane, layer);
                SVDEBUG << "OSCHandler: Set current layer to index "
                        << layerIndex << " (layer \""
                        << layer->getLayerPresentationName() << "\")" << endl;
            } else if (wantLayer && layerIndex == -1) {
                m_paneStack->setCurrentLayer(pane, nullptr);
            } else if (wantLayer) {
                SVCERR << "OSCHandler: Layer index "
                       << layerIndex << " out of range for pane" << endl;
            }                
        }

    } else if (message.getMethod() == "delete") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {
            
            QString target = message.getArg(0).toString();

            if (target == "pane") {

                SVDEBUG << "OSCHandler: Calling deleteCurrentPane" << endl;
                deleteCurrentPane();

            } else if (target == "layer") {

                SVDEBUG << "OSCHandler: Calling deleteCurrentLayer" << endl;
                deleteCurrentLayer();

            } else {
                
                SVCERR << "WARNING: OSCHandler: Unknown delete target \""
                       << target << "\"" << endl;
            }
        } else {
            SVCERR << "OSCHandler: Usage: /delete pane" << endl
                   << "               or  /delete layer" << endl;
        }

    } else if (message.getMethod() == "zoom") {

        if (message.getArgCount() == 1) {
            if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                message.getArg(0).toString() == "in") {
                zoomIn();
            } else if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                       message.getArg(0).toString() == "out") {
                zoomOut();
            } else if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                       message.getArg(0).toString() == "default") {
                zoomDefault();
            } else if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                       message.getArg(0).toString() == "fit") {
                zoomToFit();
            } else if (message.getArg(0).canConvert(QMetaType(QMetaType::Double))) {
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
                    SVDEBUG << "OSCHandler: Setting zoom level to "
                            << zoomLevel << endl;
                    currentPane->setZoomLevel(zoomLevel);
                } else {
                    SVCERR << "OSCHandler: No current pane, can't set zoom"
                           << endl;
                }
            }
        } else {
            SVCERR << "OSCHandler: Usage: /zoom <level>" << endl
                   << "               or  /zoom in" << endl
                   << "               or  /zoom out" << endl
                   << "               or  /zoom fit" << endl
                   << "               or  /zoom default" << endl;
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
                if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                    message.getArg(0).toString() == "in") {
                    int step = layer->getCurrentVerticalZoomStep() + 1;
                    if (step < steps) layer->setVerticalZoomStep(step);
                } else if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                           message.getArg(0).toString() == "out") {
                    int step = layer->getCurrentVerticalZoomStep() - 1;
                    if (step >= 0) layer->setVerticalZoomStep(step);
                } else if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                           message.getArg(0).toString() == "default") {
                    layer->setVerticalZoomStep(defaultStep);
                }
            } else if (message.getArgCount() == 2) {
                if (message.getArg(0).canConvert(QMetaType(QMetaType::Double)) &&
                    message.getArg(1).canConvert(QMetaType(QMetaType::Double))) {
                    double min = message.getArg(0).toDouble();
                    double max = message.getArg(1).toDouble();
                    layer->setDisplayExtents(min, max);
                }
            }
        }

    } else if (message.getMethod() == "quit") {

        SVDEBUG << "OSCHandler: Exiting abruptly" << endl;

        // discard any more pending OSC messages
        if (m_oscQueue) {
            while (!m_oscQueue->isEmpty()) {
                (void)m_oscQueue->readMessage();
            }
        }
        
        m_documentModified = false; // so we don't ask to save
        close();

    } else if (message.getMethod() == "resize") {
        
        if (message.getArgCount() == 2) {

            int width = 0, height = 0;

            if (message.getArg(1).canConvert(QMetaType(QMetaType::Int))) {

                height = message.getArg(1).toInt();

                if (message.getArg(0).canConvert(QMetaType(QMetaType::QString)) &&
                    message.getArg(0).toString() == "pane") {

                    Pane *pane = m_paneStack->getCurrentPane();
                    if (pane) pane->resize(pane->width(), height);

                } else if (message.getArg(0).canConvert(QMetaType(QMetaType::Int))) {

                    width = message.getArg(0).toInt();
                    resize(width, height);
                }
            }
        }

    } else if (message.getMethod() == "transform") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QMetaType(QMetaType::QString))) {

            Pane *pane = m_paneStack->getCurrentPane();
            
            if (getMainModel() && pane) {

                TransformId transformId = message.getArg(0).toString();

                Transform transform = TransformFactory::getInstance()->
                    getDefaultTransformFor(transformId);

                SVDEBUG << "OSCHandler: Running transform on main model:"
                        << transform.toXmlString() << endl;
            
                Layer *newLayer = m_document->createDerivedLayer
                    (transform, getMainModelId());
                
                if (newLayer) {
                    m_document->addLayerToView(pane, newLayer);
                    m_recentTransforms.add(transformId);
                    m_paneStack->setCurrentLayer(pane, newLayer);
                } else {
                    SVCERR << "OSCHandler: Transform failed to run" << endl;
                }
            } else {
                SVCERR << "OSCHandler: Lack main model or pane, "
                       << "can't run transform" << endl;
            }
        }            

    } else {
        SVCERR << "WARNING: OSCHandler: Unknown or unsupported "
                  << "method \"" << message.getMethod()
                  << "\"" << endl;
    }

    SVDEBUG << "OSCHandler at " << NOW << ": finished message: "
            << message.toString() << " in " << timer.elapsed() << "ms" << endl;
}
