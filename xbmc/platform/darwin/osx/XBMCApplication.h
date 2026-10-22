/*
 *  XBMCApplication.h
 *  Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
 *  Non-NIB-Code & other changes: Max Horn <max@quendi.de>
 *
 *  SPDX-License-Identifier: Unlicense
 *  See LICENSES/README.md for more information.
 */

#import <Cocoa/Cocoa.h>

@interface XBMCDelegate : NSObject<NSApplicationDelegate>
//! Set once the window has been hidden for exiting. Launching the app from
//! then on relaunches it after it has exited.
@property(nonatomic) BOOL exiting;
@end
