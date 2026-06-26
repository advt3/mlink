package gui

/*
#cgo LDFLAGS: -framework Cocoa
#include <stdlib.h>
#include "cocoa_gui.h"
*/
import "C"
import (
	"unsafe"
	"desk-link/internal/config"
)

// Start launches the native Cocoa event loop and blocks.
func Start(cfg config.UIConfig) {
	cColor := C.CString(cfg.TextColor)
	defer C.free(unsafe.Pointer(cColor))

	C.StartCocoaApp(
		C.int(cfg.Width),
		C.int(cfg.Height),
		C.int(cfg.X),
		C.int(cfg.Y),
		C.int(cfg.FontSize),
		cColor,
	)
}

// UpdateText updates the displayed widget text on the transparent desktop.
func UpdateText(text string) {
	cText := C.CString(text)
	defer C.free(unsafe.Pointer(cText))
	C.UpdateWidgetText(cText)
}
