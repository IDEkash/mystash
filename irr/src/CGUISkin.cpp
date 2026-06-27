// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2019 Irrlick
//
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUISkin.h"

#include "IGUIFont.h"
#include "IGUISpriteBank.h"
#include "IGUIElement.h"
#include "IVideoDriver.h"
#include "guiutil.h"

namespace gui
{

CGUISkin::CGUISkin(video::IVideoDriver* driver)
: SpriteBank(0), Driver(driver)
{
	Colors[EGDC_3D_DARK_SHADOW]     = video::SColor(150,0,0,0);
	Colors[EGDC_3D_SHADOW]          = video::SColor(120,40,45,50);
	Colors[EGDC_3D_FACE]            = video::SColor(150,70,75,80);
	Colors[EGDC_3D_HIGH_LIGHT]      = video::SColor(160,200,205,210);
	Colors[EGDC_3D_LIGHT]           = video::SColor(100,100,105,110);
	Colors[EGDC_ACTIVE_BORDER]      = video::SColor(200,80,120,220);
	Colors[EGDC_ACTIVE_CAPTION]     = video::SColor(255,255,255,255);
	Colors[EGDC_APP_WORKSPACE]      = video::SColor(150,30,30,30);
	Colors[EGDC_BUTTON_TEXT]        = video::SColor(255,250,250,250);
	Colors[EGDC_GRAY_TEXT]          = video::SColor(255,150,155,160);
	Colors[EGDC_HIGH_LIGHT]         = video::SColor(200,90,150,255);
	Colors[EGDC_HIGH_LIGHT_TEXT]    = video::SColor(255,255,255,255);
	Colors[EGDC_INACTIVE_BORDER]    = video::SColor(120,50,55,60);
	Colors[EGDC_INACTIVE_CAPTION]   = video::SColor(255,190,195,200);
	Colors[EGDC_TOOLTIP]            = video::SColor(255,255,255,255);
	Colors[EGDC_TOOLTIP_BACKGROUND] = video::SColor(240,40,40,40);
	Colors[EGDC_SCROLLBAR]          = video::SColor(140,50,55,60);
	Colors[EGDC_WINDOW]             = video::SColor(160,35,35,35);
	Colors[EGDC_WINDOW_SYMBOL]      = video::SColor(255,240,245,250);
	Colors[EGDC_ICON]               = video::SColor(255,255,255,255);
	Colors[EGDC_ICON_HIGH_LIGHT]    = video::SColor(220,110,170,255);
	Colors[EGDC_GRAY_WINDOW_SYMBOL] = video::SColor(255,130,135,140);
	Colors[EGDC_EDITABLE] 			= video::SColor(150,20,20,25);
	Colors[EGDC_GRAY_EDITABLE]		= video::SColor(120,45,47,50);
	Colors[EGDC_FOCUSED_EDITABLE]	= video::SColor(180,60,110,210);


	Sizes[EGDS_SCROLLBAR_SIZE] = 14;
	Sizes[EGDS_MENU_HEIGHT] = 30;
	Sizes[EGDS_WINDOW_BUTTON_WIDTH] = 15;
	Sizes[EGDS_CHECK_BOX_WIDTH] = 18;
	Sizes[EGDS_MESSAGE_BOX_WIDTH] = 500;
	Sizes[EGDS_MESSAGE_BOX_HEIGHT] = 200;
	Sizes[EGDS_BUTTON_WIDTH] = 80;
	Sizes[EGDS_BUTTON_HEIGHT] = 30;
	Sizes[EGDS_BORDER_RADIUS] = 6;

	Sizes[EGDS_TEXT_DISTANCE_X] = 2;
	Sizes[EGDS_TEXT_DISTANCE_Y] = 0;

	Sizes[EGDS_TITLEBARTEXT_DISTANCE_X] = 2;
	Sizes[EGDS_TITLEBARTEXT_DISTANCE_Y] = 0;
	Sizes[EGDS_MESSAGE_BOX_GAP_SPACE] = 15;
	Sizes[EGDS_MESSAGE_BOX_MIN_TEXT_WIDTH] = 0;
	Sizes[EGDS_MESSAGE_BOX_MAX_TEXT_WIDTH] = 500;
	Sizes[EGDS_MESSAGE_BOX_MIN_TEXT_HEIGHT] = 0;
	Sizes[EGDS_MESSAGE_BOX_MAX_TEXT_HEIGHT] = 99999;

	Sizes[EGDS_BUTTON_PRESSED_IMAGE_OFFSET_X] = 1;
	Sizes[EGDS_BUTTON_PRESSED_IMAGE_OFFSET_Y] = 1;
	Sizes[EGDS_BUTTON_PRESSED_TEXT_OFFSET_X] = 0;
	Sizes[EGDS_BUTTON_PRESSED_TEXT_OFFSET_Y] = 2;

	Texts[EGDT_MSG_BOX_OK] = L"OK";
	Texts[EGDT_MSG_BOX_CANCEL] = L"Cancel";
	Texts[EGDT_MSG_BOX_YES] = L"Yes";
	Texts[EGDT_MSG_BOX_NO] = L"No";
	Texts[EGDT_WINDOW_CLOSE] = L"Close";
	Texts[EGDT_WINDOW_RESTORE] = L"Restore";
	Texts[EGDT_WINDOW_MINIMIZE] = L"Minimize";
	Texts[EGDT_WINDOW_MAXIMIZE] = L"Maximize";

	Icons[EGDI_WINDOW_MAXIMIZE] = 225;
	Icons[EGDI_WINDOW_RESTORE] = 226;
	Icons[EGDI_WINDOW_CLOSE] = 227;
	Icons[EGDI_WINDOW_MINIMIZE] = 228;
	Icons[EGDI_CURSOR_UP] = 229;
	Icons[EGDI_CURSOR_DOWN] = 230;
	Icons[EGDI_CURSOR_LEFT] = 231;
	Icons[EGDI_CURSOR_RIGHT] = 232;
	Icons[EGDI_MENU_MORE] = 232;
	Icons[EGDI_CHECK_BOX_CHECKED] = 233;
	Icons[EGDI_DROP_DOWN] = 234;
	Icons[EGDI_SMALL_CURSOR_UP] = 235;
	Icons[EGDI_SMALL_CURSOR_DOWN] = 236;
	Icons[EGDI_RADIO_BUTTON_CHECKED] = 237;
	Icons[EGDI_MORE_LEFT] = 238;
	Icons[EGDI_MORE_RIGHT] = 239;
	Icons[EGDI_MORE_UP] = 240;
	Icons[EGDI_MORE_DOWN] = 241;
	Icons[EGDI_WINDOW_RESIZE] = 242;
	Icons[EGDI_EXPAND] = 243;
	Icons[EGDI_COLLAPSE] = 244;

	Icons[EGDI_FILE] = 245;
	Icons[EGDI_DIRECTORY] = 246;

	for (u32 i=0; i<EGDF_COUNT; ++i)
		Fonts[i] = 0;
}


//! destructor
CGUISkin::~CGUISkin()
{
	for (u32 i=0; i<EGDF_COUNT; ++i)
	{
		if (Fonts[i])
			Fonts[i]->drop();
	}

	if (SpriteBank)
		SpriteBank->drop();
}


//! returns default color
video::SColor CGUISkin::getColor(EGUI_DEFAULT_COLOR color) const
{
	if ((u32)color < EGDC_COUNT)
		return Colors[color];
	else
		return video::SColor();
}


//! sets a default color
void CGUISkin::setColor(EGUI_DEFAULT_COLOR which, video::SColor newColor)
{
	if ((u32)which < EGDC_COUNT)
		Colors[which] = newColor;
}


//! returns size for the given size type
s32 CGUISkin::getSize(EGUI_DEFAULT_SIZE size) const
{
	if ((u32)size < EGDS_COUNT)
		return Sizes[size];
	else
		return 0;
}


//! sets a default size
void CGUISkin::setSize(EGUI_DEFAULT_SIZE which, s32 size)
{
	if ((u32)which < EGDS_COUNT)
		Sizes[which] = size;
}


//! returns the default font
IGUIFont* CGUISkin::getFont(EGUI_DEFAULT_FONT which) const
{
	if (((u32)which < EGDF_COUNT) && Fonts[which])
		return Fonts[which];
	else
		return Fonts[EGDF_DEFAULT];
}


//! sets a default font
void CGUISkin::setFont(IGUIFont* font, EGUI_DEFAULT_FONT which)
{
	if ((u32)which >= EGDF_COUNT)
		return;

	if (font)
	{
		font->grab();
		if (Fonts[which])
			Fonts[which]->drop();

		Fonts[which] = font;
	}
}


//! gets the sprite bank stored
IGUISpriteBank* CGUISkin::getSpriteBank() const
{
	return SpriteBank;
}


//! set a new sprite bank or remove one by passing 0
void CGUISkin::setSpriteBank(IGUISpriteBank* bank)
{
	if (bank)
		bank->grab();

	if (SpriteBank)
		SpriteBank->drop();

	SpriteBank = bank;
}


//! Returns a default icon
u32 CGUISkin::getIcon(EGUI_DEFAULT_ICON icon) const
{
	if ((u32)icon < EGDI_COUNT)
		return Icons[icon];
	else
		return 0;
}


//! Sets a default icon
void CGUISkin::setIcon(EGUI_DEFAULT_ICON icon, u32 index)
{
	if ((u32)icon < EGDI_COUNT)
		Icons[icon] = index;
}


//! Returns a default text. For example for Message box button captions:
//! "OK", "Cancel", "Yes", "No" and so on.
const wchar_t* CGUISkin::getDefaultText(EGUI_DEFAULT_TEXT text) const
{
	if ((u32)text < EGDT_COUNT)
		return Texts[text].c_str();
	else
		return Texts[0].c_str();
}


//! Sets a default text. For example for Message box button captions:
//! "OK", "Cancel", "Yes", "No" and so on.
void CGUISkin::setDefaultText(EGUI_DEFAULT_TEXT which, const wchar_t* newText)
{
	if ((u32)which < EGDT_COUNT)
		Texts[which] = newText;
}


//! draws a standard 3d button pane
/**	Used for drawing for example buttons in normal state.
It uses the colors EGDC_3D_DARK_SHADOW, EGDC_3D_HIGH_LIGHT, EGDC_3D_SHADOW and
EGDC_3D_FACE for this. See EGUI_DEFAULT_COLOR for details.
\param rect: Defining area where to draw.
\param clip: Clip area.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly. */
// PATCH
void CGUISkin::drawColored3DButtonPaneStandard(IGUIElement* element,
					const core::rect<s32>& r,
					const core::rect<s32>* clip,
					const video::SColor* colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	draw2DRoundedRectangle(Driver, r, colors[EGDC_3D_FACE], getSize(EGDS_BORDER_RADIUS), clip);
}
// END PATCH


//! draws a pressed 3d button pane
/**	Used for drawing for example buttons in pressed state.
It uses the colors EGDC_3D_DARK_SHADOW, EGDC_3D_HIGH_LIGHT, EGDC_3D_SHADOW and
EGDC_3D_FACE for this. See EGUI_DEFAULT_COLOR for details.
\param rect: Defining area where to draw.
\param clip: Clip area.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly. */
// PATCH
void CGUISkin::drawColored3DButtonPanePressed(IGUIElement* element,
					const core::rect<s32>& r,
					const core::rect<s32>* clip,
					const video::SColor* colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	draw2DRoundedRectangle(Driver, r, colors[EGDC_HIGH_LIGHT], getSize(EGDS_BORDER_RADIUS), clip);
}
// END PATCH


//! draws a sunken 3d pane
/** Used for drawing the background of edit, combo or check boxes.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly.
\param bgcolor: Background color.
\param flat: Specifies if the sunken pane should be flat or displayed as sunken
deep into the ground.
\param rect: Defining area where to draw.
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColored3DSunkenPane(IGUIElement* element, video::SColor bgcolor,
				bool flat, bool fillBackGround,
				const core::rect<s32>& r,
				const core::rect<s32>* clip,
				const video::SColor* colors)
{
	if (!Driver)
		return;

	if (fillBackGround) {
		draw2DRoundedRectangle(Driver, r, bgcolor, getSize(EGDS_BORDER_RADIUS), clip);
	}
}
// END PATCH

//! draws a window background
// return where to draw title bar text.
// PATCH
core::rect<s32> CGUISkin::drawColored3DWindowBackground(IGUIElement* element,
				bool drawTitleBar, video::SColor titleBarColor,
				const core::rect<s32>& r,
				const core::rect<s32>* clip,
				core::rect<s32>* checkClientArea,
				const video::SColor* colors)
{
	if (!Driver)
	{
		if ( checkClientArea )
		{
			*checkClientArea = r;
		}
		return r;
	}

	if (!colors)
		colors = Colors;

	if ( !checkClientArea )
	{
		draw2DRoundedRectangle(Driver, r, colors[EGDC_3D_FACE], getSize(EGDS_BORDER_RADIUS), clip, colors[EGDC_3D_HIGH_LIGHT], 1);
	}

	core::rect<s32> rect = r;

	// client area for background
	rect.UpperLeftCorner.X +=1;
	rect.UpperLeftCorner.Y +=1;
	rect.LowerRightCorner.X -= 2;
	rect.LowerRightCorner.Y -= 2;
	if (checkClientArea)
	{
		*checkClientArea = rect;
	}

	// title bar
	rect = r;
	rect.UpperLeftCorner.X += 2;
	rect.UpperLeftCorner.Y += 2;
	rect.LowerRightCorner.X -= 2;
	rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + getSize(EGDS_WINDOW_BUTTON_WIDTH) + 2;

	if (drawTitleBar )
	{
		if (checkClientArea)
		{
			(*checkClientArea).UpperLeftCorner.Y = rect.LowerRightCorner.Y;
		}
		else
		{
			// draw title bar
			draw2DRoundedRectangle(Driver, rect, titleBarColor, getSize(EGDS_BORDER_RADIUS), clip);
		}
	}

	return rect;
}
// END PATCH


//! draws a standard 3d menu pane
/**	Used for drawing for menus and context menus.
It uses the colors EGDC_3D_DARK_SHADOW, EGDC_3D_HIGH_LIGHT, EGDC_3D_SHADOW and
EGDC_3D_FACE for this. See EGUI_DEFAULT_COLOR for details.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly.
\param rect: Defining area where to draw.
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColored3DMenuPane(IGUIElement *element,
		const core::rect<s32> &r, const core::rect<s32> *clip,
		const video::SColor *colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	draw2DRoundedRectangle(Driver, r, colors[EGDC_3D_FACE], getSize(EGDS_BORDER_RADIUS), clip, colors[EGDC_3D_HIGH_LIGHT], 1);
}
// END PATCH


//! draws a standard 3d tool bar
/**	Used for drawing for toolbars and menus.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly.
\param rect: Defining area where to draw.
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColored3DToolBar(IGUIElement* element,
				const core::rect<s32>& r,
				const core::rect<s32>* clip,
				const video::SColor* colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	draw2DRoundedRectangle(Driver, r, colors[EGDC_3D_FACE], getSize(EGDS_BORDER_RADIUS), clip);
}
// END PATCH

//! draws a tab button
/**	Used for drawing for tab buttons on top of tabs.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly.
\param active: Specifies if the tab is currently active.
\param rect: Defining area where to draw.
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColored3DTabButton(IGUIElement* element, bool active,
	const core::rect<s32>& frameRect, const core::rect<s32>* clip, EGUI_ALIGNMENT alignment,
	const video::SColor* colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	video::SColor color = colors[EGDC_3D_FACE];
	if (active)
		color = colors[EGDC_HIGH_LIGHT];

	draw2DRoundedRectangle(Driver, frameRect, color, getSize(EGDS_BORDER_RADIUS), clip);
}
// END PATCH


//! draws a tab control body
/**	\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly.
\param border: Specifies if the border should be drawn.
\param background: Specifies if the background should be drawn.
\param rect: Defining area where to draw.
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColored3DTabBody(IGUIElement* element, bool border, bool background,
	const core::rect<s32>& rect, const core::rect<s32>* clip, s32 tabHeight, EGUI_ALIGNMENT alignment,
	const video::SColor* colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	if (background)
	{
		draw2DRoundedRectangle(Driver, rect, colors[EGDC_3D_FACE], getSize(EGDS_BORDER_RADIUS), clip);
	}
}
// END PATCH


//! draws an icon, usually from the skin's sprite bank
/**	\param parent: Pointer to the element which wishes to draw this icon.
This parameter is usually not used by IGUISkin, but can be used for example
by more complex implementations to find out how to draw the part exactly.
\param icon: Specifies the icon to be drawn.
\param position: The position to draw the icon
\param starttime: The time at the start of the animation
\param currenttime: The present time, used to calculate the frame number
\param loop: Whether the animation should loop or not
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColoredIcon(IGUIElement* element, EGUI_DEFAULT_ICON icon,
			const core::position2di position,
			u32 starttime, u32 currenttime,
			bool loop, const core::rect<s32>* clip,
			const video::SColor* colors)
{
	if (!SpriteBank)
		return;

	if (!colors)
		colors = Colors;

	bool gray = element && !element->isEnabled();
	SpriteBank->draw2DSprite(Icons[icon], position, clip,
			colors[gray? EGDC_GRAY_WINDOW_SYMBOL : EGDC_WINDOW_SYMBOL], starttime, currenttime, loop, true);
}
// END PATCH


//! draws a 2d rectangle.
void CGUISkin::draw2DRectangle(IGUIElement* element,
		const video::SColor &color, const core::rect<s32>& pos,
		const core::rect<s32>* clip)
{
	if (pos.getWidth() == pos.getHeight() && pos.getWidth() < 32)
		draw2DRoundedRectangle(Driver, pos, color, 4, clip);
	else
		draw2DRoundedRectangle(Driver, pos, color, 2, clip);
}


//! gets the colors
// PATCH
void CGUISkin::getColors(video::SColor* colors)
{
	u32 i;
	for (i=0; i<EGDC_COUNT; ++i)
		colors[i] = Colors[i];
}
// END PATCH

} // end namespace gui
