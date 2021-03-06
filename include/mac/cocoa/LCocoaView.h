//
//  LCocoaView.h
//  LgiCocoa
//
//  Created by Matthew Allen on 20/10/18.
//  Copyright © 2018 Memecode. All rights reserved.
//

#ifndef LCocoaView_h
#define LCocoaView_h

// This class wraps a Cocoa NSView and redirects all the calls to LGI's GView object.
@interface LCocoaView : NSView
{
}
@property GView *v;
- (id)init:(GView*)view;
- (void)dealloc;
- (void)drawRect:(NSRect)dirtyRect;
- (void)layout;
- (void)mouseDown:(NSEvent*)ev;
- (void)mouseUp:(NSEvent*)ev;
- (void)rightMouseDown:(NSEvent*)ev;
- (void)rightMouseUp:(NSEvent*)ev;
- (void)mouseMoved:(NSEvent*)ev;
@end


#endif /* LCocoaView_h */
