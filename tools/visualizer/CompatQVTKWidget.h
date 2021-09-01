#if (VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION >= 2) || VTK_MAJOR_VERSION > 8
#include "QVTKOpenGLNativeWidget.h"
#define CompatQVTKWidget QVTKOpenGLNativeWidget
#else
#include "QVTKWidget.h"
#define CompatQVTKWidget QVTKWidget
#endif
