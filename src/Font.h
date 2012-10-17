#ifndef _FONT_H_
#define _FONT_H_

#include <string>
#include "platform.h"
#include GLHEADER
#include <ft2build.h>
#include FT_FREETYPE_H

//A TrueType Font renderer that uses FreeType and OpenGL. initLibrary() MUST be called before using it.
class Font
{
public:
	static void initLibrary();

	Font(std::string path, int size);
	~Font();

	FT_Face face;

	//contains sizing information for every glyph.
	struct charPosData {
		int texX;
		int texY;
		int texW;
		int texH;

		int advX;
		int advY;

		int bearingY;
	};

	charPosData charData[128];

	GLuint textureID;

	void drawText(std::string text, int startx, int starty, int color, char opacity); //Render some text using this font.
	void sizeText(std::string text, int* w, int* h); //Sets the width and height of a given string to given pointers. Skipped if pointer is NULL.

private:
	static int getDpiX();
	static int getDpiY();
	static FT_Library sLibrary;

	void buildAtlas(); //Builds a "texture atlas," one big OpenGL texture with glyphs 32 to 128.

	int textureWidth; //OpenGL texture width
	int textureHeight; //OpenGL texture height
	int mMaxGlyphHeight;
};

#endif
