#include "Animation.h"

namespace desktoprobot
{
    void drawFrame(const Frame& frame, Framebuffer& framebuffer)
    {
        for (std::size_t index = 0; index < frame.commandCount; ++index)
        {
            DrawCommand command;
            if (frame.packedCommands) {
                const auto& packed = frame.packedCommands[index];
                command = {static_cast<DrawCommandType>(packed.typeAndFilled & 0x7f),
                    packed.x1, packed.y1, packed.x2, packed.y2,
                    (packed.typeAndFilled & 0x80) != 0, packed.radius};
            } else command = frame.commands[index];

            switch (command.type)
            {
            case DrawCommandType::Pixel:
                framebuffer.setPixel(command.x1, command.y1);
                break;

            case DrawCommandType::Rectangle:
                framebuffer.drawRect(command.x1, command.y1, command.x2, command.y2);
                break;

            case DrawCommandType::RoundedRectangle:
                framebuffer.drawRoundedRect(
                    command.x1,
                    command.y1,
                    command.x2,
                    command.y2,
                    command.radius,
                    command.filled
                );
                break;

            case DrawCommandType::Line:
                framebuffer.drawLine(command.x1, command.y1, command.x2, command.y2, command.filled);
                break;

            case DrawCommandType::Circle:
                framebuffer.drawCircle(command.x1, command.y1, command.x2, command.filled);
                break;
            }
        }
    }
}
