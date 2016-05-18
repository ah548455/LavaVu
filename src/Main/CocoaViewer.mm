/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
** Copyright (c) 2010, Monash University
** All rights reserved.
** Redistribution and use in source and binary forms, with or without modification,
** are permitted provided that the following conditions are met:
**
**       * Redistributions of source code must retain the above copyright notice,
**          this list of conditions and the following disclaimer.
**       * Redistributions in binary form must reproduce the above copyright
**         notice, this list of conditions and the following disclaimer in the
**         documentation and/or other materials provided with the distribution.
**       * Neither the name of the Monash University nor the names of its contributors
**         may be used to endorse or promote products derived from this software
**         without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
** THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
** OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**
** Contact:
*%  Owen Kaluza - Owen.Kaluza(at)monash.edu
*%
*% Development Team :
*%  http://www.underworldproject.org/aboutus.html
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifdef HAVE_CGL

#include "CocoaViewer.h"

OpenGLViewer* _viewer = NULL;
bool redisplay = true;

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CVDisplayLink.h>
#import <OpenGL/OpenGL.h>

@class CView;
static CVReturn GlobalDisplayLinkCallback(CVDisplayLinkRef, const CVTimeStamp*, const CVTimeStamp*, CVOptionFlags, CVOptionFlags*, void*);

@interface CView : NSOpenGLView <NSWindowDelegate>
{
@public
  CVDisplayLinkRef displayLink;
  bool running;
  NSRect windowRect;
  NSRecursiveLock* appLock;
}
@end

@implementation CView
// Initialize
- (id) initWithFrame: (NSRect) frame
{
  running = true;

  // multisampling
  int samples = 4;

  // Keep multisampling attributes at the start of the attribute lists since code below assumes they are array elements 0 through 4.
  NSOpenGLPixelFormatAttribute windowedAttrs[] =
  {
    NSOpenGLPFAMultisample,
    NSOpenGLPFASampleBuffers, 1,
    NSOpenGLPFASamples, 4,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAColorSize, 32,
    NSOpenGLPFADepthSize, 24,
    NSOpenGLPFAAlphaSize, 8,
    NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
    0
  };

  // Try to choose a supported pixel format
  NSOpenGLPixelFormat* pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:windowedAttrs];

  if (!pf)
  {
    bool valid = false;
    while (!pf && samples > 0)
    {
      samples /= 2;
      windowedAttrs[2] = samples ? 1 : 0;
      windowedAttrs[4] = samples;
      pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:windowedAttrs];
      if (pf)
      {
        valid = true;
        break;
      }
    }

    if (!valid)
    {
      NSLog(@"OpenGL pixel format not supported.");
      return nil;
    }
  }

  self = [super initWithFrame:frame pixelFormat:[pf autorelease]];
  appLock = [[NSRecursiveLock alloc] init];

  return self;
}

- (void) prepareOpenGL
{
  [super prepareOpenGL];

  [[self window] setLevel: NSNormalWindowLevel];
  [[self window] makeKeyAndOrderFront: self];

  // Make all the OpenGL calls to setup rendering and build the necessary rendering objects
  [[self openGLContext] makeCurrentContext];
  // Synchronize buffer swaps with vertical refresh rate
  GLint swapInt = 1; // Vsynch on!
  [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

  // Create a display link capable of being used with all active displays
  CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);

  // Set the renderer output callback function
  CVDisplayLinkSetOutputCallback(displayLink, &GlobalDisplayLinkCallback, self);

  CGLContextObj cglContext = (CGLContextObj)[[self openGLContext] CGLContextObj];
  CGLPixelFormatObj cglPixelFormat = (CGLPixelFormatObj)[[self pixelFormat] CGLPixelFormatObj];
  CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);

  GLint dim[2] = {(int)windowRect.size.width, (int)windowRect.size.height};
  CGLSetParameter(cglContext, kCGLCPSurfaceBackingSize, dim);
  CGLEnable(cglContext, kCGLCESurfaceBackingSize);

  [appLock lock];
  CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
  NSLog(@"Initialize");

  NSLog(@"GL version:   %s", glGetString(GL_VERSION));
  NSLog(@"GLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

  //Call Viewer OpenGL init
  _viewer->init();

  // End temp
  CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
  [appLock unlock];

  // Activate the display link
  CVDisplayLinkStart(displayLink);
}

// Tell the window to accept input events
- (BOOL)acceptsFirstResponder
{
  return YES;
}

void getKeyModifiers(NSEvent* event)
{
  //Save shift states
  int modifiers = [event modifierFlags];
  _viewer->keyState.shift = (modifiers & NSShiftKeyMask);
  _viewer->keyState.ctrl = (modifiers & NSControlKeyMask);
  _viewer->keyState.alt = (modifiers & NSAlternateKeyMask);
}

- (void)mouseMoved:(NSEvent*) event
{
  [appLock lock];
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  //redisplay = _viewer->mouseMove((int)point.x, _viewer->height-(int)point.y);
  //NSLog(@"Mouse pos: %lf, %lf", point.x, point.y);
  [appLock unlock];
}

- (void) mouseDragged: (NSEvent*) event
{
  [appLock lock];
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  getKeyModifiers(event);
  //Only works for left-drag
  redisplay = _viewer->mouseMove((int)point.x, _viewer->height-(int)point.y);
  //NSLog(@"Mouse pos: %lf, %lf", point.x, point.y);
  [appLock unlock];
}

- (void)scrollWheel: (NSEvent*) event
{
  [appLock lock];
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  //MouseButton button = event.deltaY < 0 ? WheelUp : WheelDown;
  //redisplay = _viewer->mousePress(button, true, (int)point.x, _viewer->height-(int)point.y);
  getKeyModifiers(event);
  float delta = [event deltaY] / 5.0;
  if (delta != 0)
    redisplay = _viewer->mouseScroll(delta);
  //NSLog(@"Mouse wheel at: %lf, %lf. Delta: %lf", point.x, point.y, [event deltaY]);
  [appLock unlock];
}

- (void) mouseDown: (NSEvent*) event
{
  [appLock lock];
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  MouseButton button = LeftButton;
  _viewer->mouseState ^= (int)pow(2, button);
  redisplay = _viewer->mousePress(button, true, (int)point.x, _viewer->height-(int)point.y);
  //NSLog(@"Left mouse down: %lf, %lf", point.x, point.y);
  [appLock unlock];
}

- (void) mouseUp: (NSEvent*) event
{
  [appLock lock];
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  MouseButton button = LeftButton;
  _viewer->mouseState = 0;
  redisplay = _viewer->mousePress(button, false, (int)point.x, _viewer->height-(int)point.y);
  //NSLog(@"Left mouse up: %lf, %lf", point.x, point.y);
  [appLock unlock];
}

- (void) rightMouseDown: (NSEvent*) event
{
  [appLock lock];
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  MouseButton button = RightButton;
  _viewer->mouseState ^= (int)pow(2, button);
  redisplay = _viewer->mousePress(button, true, (int)point.x, _viewer->height-(int)point.y);
  //NSLog(@"Right mouse down: %lf, %lf", point.x, point.y);
  [appLock unlock];
}

- (void) rightMouseUp: (NSEvent*) event
{
  [appLock lock];
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  MouseButton button = RightButton;
  _viewer->mouseState = 0;
  redisplay = _viewer->mousePress(button, false, (int)point.x, _viewer->height-(int)point.y);
  //NSLog(@"Right mouse up: %lf, %lf", point.x, point.y);
  [appLock unlock];
}

- (void)otherMouseDown: (NSEvent*) event
{
  [appLock lock];
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  MouseButton button = MiddleButton;
  _viewer->mouseState ^= (int)pow(2, button);
  redisplay = _viewer->mousePress(button, true, (int)point.x, _viewer->height-(int)point.y);
  //NSLog(@"Middle mouse down: %lf, %lf", point.x, point.y);
  [appLock unlock];
}

- (void)otherMouseUp: (NSEvent*) event
{
  [appLock lock];
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  MouseButton button = MiddleButton;
  _viewer->mouseState = 0;
  redisplay = _viewer->mousePress(button, false, (int)point.x, _viewer->height-(int)point.y);
  //NSLog(@"Middle mouse up: %lf, %lf", point.x, point.y);
  [appLock unlock];
}

- (void) mouseEntered: (NSEvent*)event
{
  [appLock lock];
  //NSLog(@"Mouse entered");
  [appLock unlock];
}

- (void) mouseExited: (NSEvent*)event
{
  [appLock lock];
  //NSLog(@"Mouse left");
  [appLock unlock];
}

- (void) keyDown: (NSEvent*) event
{
  [appLock lock];
  if ([event isARepeat] == NO)
  {
    //NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    //NSLog(@"Key down: %d", [event keyCode]);
    //redisplay = _viewer->keyPress([event keyCode], (int)point.x, _viewer->height-(int)point.y);
  }
  [appLock unlock];
}

- (void) keyUp: (NSEvent*) event
{
  [appLock lock];
  //NSLog(@"Key up: %d", [event keyCode]);
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  NSString *keyChars = [event charactersIgnoringModifiers];
  unichar keyChar = 0;
  getKeyModifiers(event);
  if ( [keyChars length] == 1 ) {
    unsigned char key = [keyChars characterAtIndex:0];
    if ([event keyCode] == 126) key = KEY_UP;
    else if ([event keyCode] == 125) key = KEY_DOWN;
    else if ([event keyCode] == 123) key = KEY_LEFT;
    else if ([event keyCode] == 124) key = KEY_RIGHT;
    else if ([event keyCode] == 116) key = KEY_PAGEUP;
    else if ([event keyCode] == 121) key = KEY_PAGEDOWN;
    else if ([event keyCode] == 115) key = KEY_HOME;
    else if ([event keyCode] == 119) key = KEY_END;

    redisplay = _viewer->keyPress(key, (int)point.x, _viewer->height-(int)point.y);
    //redisplay = true;
  }
  [appLock unlock];
}

// Update
- (CVReturn) getFrameForTime:(const CVTimeStamp*)outputTime
{
  [appLock lock];

  [[self openGLContext] makeCurrentContext];
  CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);

  if (_viewer->postdisplay || OpenGLViewer::pollInput())
    redisplay = true;

  //NSLog(@"Update");
  if (redisplay)
    _viewer->display();
  redisplay = false;

  // EndTemp

  CGLFlushDrawable((CGLContextObj)[[self openGLContext] CGLContextObj]);
  CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);

  if (_viewer->quitProgram)   // Update loop returns false
  {
    [NSApp terminate:self];
  }

  [appLock unlock];

  return kCVReturnSuccess;
}

// Resize
- (void)windowDidResize:(NSNotification*)notification
{
  [appLock lock];
  [[self openGLContext] makeCurrentContext];
  CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);

  NSSize size = [ [ _window contentView ] frame ].size;
  NSLog(@"Window resize: %lf, %lf", size.width, size.height);
  windowRect.size.width = size.width;
  windowRect.size.height = size.height;

  //Update dims to context
  GLint dim[2] = {(int)windowRect.size.width, (int)windowRect.size.height};
  CGLContextObj cglContext = (CGLContextObj)[[self openGLContext] CGLContextObj];
  CGLSetParameter(cglContext, kCGLCPSurfaceBackingSize, dim);

  _viewer->resize(size.width, size.height);
  _viewer->postdisplay = true;
 
  CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
  [appLock unlock];
}

- (void)resumeDisplayRenderer
{
  [appLock lock];
  CVDisplayLinkStop(displayLink);
  [appLock unlock];
}

- (void)haltDisplayRenderer
{
  [appLock lock];
  CVDisplayLinkStop(displayLink);
  [appLock unlock];
}

// Terminate window when the red X is pressed
-(void)windowWillClose:(NSNotification *)notification
{
  if (running)
  {
    running = false;

    [appLock lock];
    NSLog(@"Cleanup");

    CVDisplayLinkStop(displayLink);
    CVDisplayLinkRelease(displayLink);

    [appLock unlock];
  }

  [NSApp terminate:self];
}

// Cleanup
- (void) dealloc
{
  [appLock release];
  [super dealloc];
}
@end

static CVReturn GlobalDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
  CVReturn result = [(CView*)displayLinkContext getFrameForTime:outputTime];
  return result;
}

NSAutoreleasePool * pool;
NSWindow * window;

void CocoaWindow_Open()
{
  // Autorelease Pool:
  // Objects declared in this scope will be automatically
  // released at the end of it, when the pool is "drained".
  pool = [[NSAutoreleasePool alloc] init];

  // Create a shared app instance.
  // This will initialize the global variable
  // 'NSApp' with the application instance.
  [NSApplication sharedApplication];

  // Create a window:

  // Style flags
  NSUInteger windowStyle = NSTitledWindowMask  | NSClosableWindowMask | NSResizableWindowMask | NSMiniaturizableWindowMask;

  // Window bounds (x, y, width, height)
  NSRect screenRect = [[NSScreen mainScreen] frame];
  NSRect viewRect = NSMakeRect(0, 0, _viewer->width, _viewer->height);
  NSRect windowRect = NSMakeRect(NSMidX(screenRect) - NSMidX(viewRect),
                                 NSMidY(screenRect) - NSMidY(viewRect),
                                 viewRect.size.width,
                                 viewRect.size.height);

  window = [[NSWindow alloc] initWithContentRect:windowRect
            styleMask:windowStyle
            backing:NSBackingStoreBuffered
                       defer:NO];
  [window autorelease];

  // Window controller
  NSWindowController * windowController = [[NSWindowController alloc] initWithWindow:window];
  [windowController autorelease];

  // Since Snow Leopard, programs without application bundles and Info.plist files don't get a menubar
  // and can't be brought to the front unless the presentation option is changed
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

  // Next, we need to create the menu bar. You don't need to give the first item in the menubar a name
  // (it will get the application's name automatically)
  id menubar = [[NSMenu new] autorelease];
  id appMenuItem = [[NSMenuItem new] autorelease];
  [menubar addItem:appMenuItem];
  [NSApp setMainMenu:menubar];

  // Then we add the quit item to the menu. Fortunately the action is simple since terminate: is
  // already implemented in NSApplication and the NSApplication is always in the responder chain.
  id appMenu = [[NSMenu new] autorelease];
  id appName = [[NSProcessInfo processInfo] processName];
  id quitTitle = [@"Quit " stringByAppendingString:appName];
  id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle
                      action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
  [appMenu addItem:quitMenuItem];
  [appMenuItem setSubmenu:appMenu];

  // Create app delegate to handle system events
  CView* view = [[[CView alloc] initWithFrame:windowRect] autorelease];
  view->windowRect = windowRect;
  [window setAcceptsMouseMovedEvents:YES];
  [window setContentView:view];
  [window setDelegate:view];

  // Set app title
  [window setTitle:appName];

  // Add fullscreen button
  [window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];

  debug_print("Cocoa viewer created\n");

  // Show window
  [window orderFrontRegardless];
}

void CocoaWindow_Execute()
{
  //Run event loop
  _viewer->postdisplay = true;
  [NSApp run];
}

void CocoaWindow_Setsize(int width, int height)
{
  //Doing this in pool fixes warnings, but could still be threading issues here
  //causing intermittant problems...
  NSAutoreleasePool * newpool = [[NSAutoreleasePool alloc] init];

  //Resize to specified size
  NSSize size = [ [ window contentView ] frame ].size;
  NSRect frame = [window frame];

  if (size.width != width && size.height != height)
  {
    //Calc and add frame decoration sizes
    float xdiff = frame.size.width - size.width;
    float ydiff = frame.size.height - size.height;

    frame.size.width = width + xdiff;
    frame.size.height = height + ydiff;

    [window setFrame:frame display:YES animate:NO];
  }
  [newpool release];
}

void CocoaWindow_FullScreen()
{
  [window toggleFullScreen:nil];
}

void CocoaWindow_Close()
{
  [pool drain];
}

CocoaViewer::CocoaViewer() : CGLViewer()
{
  _viewer = this;
  visible = true; //override
}

CocoaViewer::~CocoaViewer()
{
  CocoaWindow_Close();
}

void CocoaViewer::open(int w, int h)
{
  if (!visible) 
  {
    //Use CGL viewer for offscreen
    CGLViewer::open(w, h);
  }
  else
  {
    //Call base class open to set width/height
    OpenGLViewer::open(w, h);
    CocoaWindow_Open();
  }
}

void CocoaViewer::setsize(int width, int height)
{
  OpenGLViewer::setsize(width, height);
  if (visible)
    CocoaWindow_Setsize(width, height);
}

void CocoaViewer::show()
{
  OpenGLViewer::show();
}

void CocoaViewer::display()
{
  OpenGLViewer::display();
  //swap();
}

void CocoaViewer::swap()
{
  // Swap buffers
}

void CocoaViewer::execute()
{
  if (visible) 
    CocoaWindow_Execute();
  else
    OpenGLViewer::execute();
}

void CocoaViewer::fullScreen()
{
  if (visible)
    CocoaWindow_FullScreen();
}
#endif   //HAVE_CGL
