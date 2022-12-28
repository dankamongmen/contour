/**
 * This file is part of the "libterminal" project
 *   Copyright (c) 2019-2020 Christian Parpart <christian@parpart.family>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vtbackend/Terminal.h>
#include <vtbackend/TerminalState.h>

namespace terminal
{

class Terminal;

TerminalState::TerminalState(Terminal& _terminal):
    settings { _terminal.settings() },
    cellPixelSize {},
    effectiveImageCanvasSize { settings.maxImageSize },
    imageColorPalette { std::make_shared<SixelColorPalette>(maxImageColorRegisters, maxImageColorRegisters) },
    imagePool { [&](Image const* _image) {
        _terminal.discardImage(*_image);
    } },
    statusDisplayType { StatusDisplayType::None },
    activeStatusDisplay { ActiveStatusDisplay::Main },
    hyperlinks { HyperlinkCache { 1024 } },
    sequencer { _terminal },
    parser { std::ref(sequencer) },
    viCommands { _terminal },
    inputHandler { viCommands, ViMode::Insert }
{
}

/// Applies a FunctionDefinition to a given context, emitting the respective command.
std::string to_string(AnsiMode _mode)
{
    switch (_mode)
    {
        case AnsiMode::KeyboardAction: return "KeyboardAction";
        case AnsiMode::Insert: return "Insert";
        case AnsiMode::SendReceive: return "SendReceive";
        case AnsiMode::AutomaticNewLine: return "AutomaticNewLine";
    }

    return fmt::format("({})", static_cast<unsigned>(_mode));
}

std::string to_string(DECMode _mode)
{
    switch (_mode)
    {
        case DECMode::UseApplicationCursorKeys: return "UseApplicationCursorKeys";
        case DECMode::DesignateCharsetUSASCII: return "DesignateCharsetUSASCII";
        case DECMode::Columns132: return "Columns132";
        case DECMode::SmoothScroll: return "SmoothScroll";
        case DECMode::ReverseVideo: return "ReverseVideo";
        case DECMode::MouseProtocolX10: return "MouseProtocolX10";
        case DECMode::MouseProtocolNormalTracking: return "MouseProtocolNormalTracking";
        case DECMode::MouseProtocolHighlightTracking: return "MouseProtocolHighlightTracking";
        case DECMode::MouseProtocolButtonTracking: return "MouseProtocolButtonTracking";
        case DECMode::MouseProtocolAnyEventTracking: return "MouseProtocolAnyEventTracking";
        case DECMode::SaveCursor: return "SaveCursor";
        case DECMode::ExtendedAltScreen: return "ExtendedAltScreen";
        case DECMode::Origin: return "Origin";
        case DECMode::AutoWrap: return "AutoWrap";
        case DECMode::PrinterExtend: return "PrinterExtend";
        case DECMode::LeftRightMargin: return "LeftRightMargin";
        case DECMode::ShowToolbar: return "ShowToolbar";
        case DECMode::BlinkingCursor: return "BlinkingCursor";
        case DECMode::VisibleCursor: return "VisibleCursor";
        case DECMode::ShowScrollbar: return "ShowScrollbar";
        case DECMode::AllowColumns80to132: return "AllowColumns80to132";
        case DECMode::DebugLogging: return "DebugLogging";
        case DECMode::UseAlternateScreen: return "UseAlternateScreen";
        case DECMode::BracketedPaste: return "BracketedPaste";
        case DECMode::FocusTracking: return "FocusTracking";
        case DECMode::NoSixelScrolling: return "NoSixelScrolling";
        case DECMode::UsePrivateColorRegisters: return "UsePrivateColorRegisters";
        case DECMode::MouseExtended: return "MouseExtended";
        case DECMode::MouseSGR: return "MouseSGR";
        case DECMode::MouseURXVT: return "MouseURXVT";
        case DECMode::MouseSGRPixels: return "MouseSGRPixels";
        case DECMode::MouseAlternateScroll: return "MouseAlternateScroll";
        case DECMode::MousePassiveTracking: return "MousePassiveTracking";
        case DECMode::ReportGridCellSelection: return "ReportGridCellSelection";
        case DECMode::BatchedRendering: return "BatchedRendering";
        case DECMode::Unicode: return "Unicode";
        case DECMode::TextReflow: return "TextReflow";
        case DECMode::SixelCursorNextToGraphic: return "SixelCursorNextToGraphic";
    }
    return fmt::format("({})", static_cast<unsigned>(_mode));
}

} // namespace terminal