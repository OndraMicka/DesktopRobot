#include "Animation.h"

namespace desktoprobot
{
    void drawFrame(const Frame& frame, Framebuffer& framebuffer)
    {
        for (std::size_t index = 0; index < frame.commandCount; ++index)
        {
            const DrawCommand& command = frame.commands[index];

            switch (command.type)
            {
            case DrawCommandType::Pixel:
                framebuffer.setPixel(command.x1, command.y1);
                break;

            case DrawCommandType::Rectangle:
                framebuffer.drawRect(command.x1, command.y1, command.x2, command.y2);
                break;

            case DrawCommandType::Line:
                framebuffer.drawLine(command.x1, command.y1, command.x2, command.y2);
                break;

            case DrawCommandType::Circle:
                framebuffer.drawCircle(command.x1, command.y1, command.x2, command.filled);
                break;
            }
        }
    }
}
