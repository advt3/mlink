#ifndef COCOA_GUI_H
#define COCOA_GUI_H

// StartCocoaApp starts the macOS Cocoa event loop, creating a borderless transparent window.
void StartCocoaApp(int width, int height, int x, int y, int fontSize, const char* colorHex);

// UpdateWidgetText updates the text displayed on the transparent desktop window.
void UpdateWidgetText(const char* text);

#endif
