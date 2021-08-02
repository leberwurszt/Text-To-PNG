// Text-to-Image converter
// to work around the 160 character limit

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// SDL libraries
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    const Uint8 rshift = 24;
    const Uint8 gshift = 16;
    const Uint8 bshift = 8;
    const Uint8 ashift = 0;
    const Uint32 rmask = 0xff000000;
    const Uint32 gmask = 0x00ff0000;
    const Uint32 bmask = 0x0000ff00;
    const Uint32 amask = 0x000000ff;
#else
    const Uint8 rshift = 0;
    const Uint8 gshift = 8;
    const Uint8 bshift = 16;
    const Uint8 ashift = 24;
    const Uint32 rmask = 0x000000ff;
    const Uint32 gmask = 0x0000ff00;
    const Uint32 bmask = 0x00ff0000;
    const Uint32 amask = 0xff000000;
#endif

SDL_Rect pictureOnScreen;

bool ShowPicture(SDL_Surface* surface)
{
    SDL_Texture* textureText = NULL;

    // Set texture filtering to linear
    if(!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0" ))
        std::cerr << "Warning: Linear texture filtering not enabled!" << std::endl;

    // initialize window
    SDL_Window* window = SDL_CreateWindow("Text-To-Image converter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, pictureOnScreen.w,  pictureOnScreen.h, SDL_WINDOW_OPENGL);
    if(!window)
    {
        std::cerr << SDL_GetError() << std::endl;
        return false;
    }

    SDL_Renderer* render = SDL_CreateRenderer(window, -1, SDL_RENDERER_TARGETTEXTURE);

    textureText = SDL_CreateTextureFromSurface(render, surface);

    char input = 0;
    SDL_Event event;
    bool quit = false;


    while(!quit)
    {
        SDL_RenderClear(render); // Clear Screen
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT: // event window quit
                    quit = true;
                    break;
                
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            quit = true;
                            break;
                    }
            }
        }

        SDL_RenderCopy(render, textureText, &pictureOnScreen, &pictureOnScreen);
        SDL_RenderPresent(render); // Confirm drawing on screen

    }
    SDL_DestroyTexture(textureText);
    SDL_DestroyWindow(window);
    return true;
}

bool SaveText(std::string text, const char *filename)
{
    bool success = true;

    std::ofstream textFile(filename);
    try
    {
        textFile << text;
    }

    catch(const std::exception& e)
    {
        success = false;
        std::cerr << e.what() << std::endl;
    }

    if(textFile.is_open())
        textFile.close();

    return success;
}

std::string LoadText(const char *filename)
{
    std::string text = "";

	std::ifstream file(filename, std::ifstream::binary);

	try
	{
        char c = 0;
        while(file.good())
        {
            file.read(&c, 1);
            if(c)
                text += c;
            c = 0;
        }
    }

    catch(const std::exception& e)
	{
		std::cerr << "Unable to load file" << std::endl << e.what() << std::endl;
        text = "";
    }

    if (file.is_open())
	    file.close();

    return text;
}

SDL_Surface* LoadPNG(const char *filename)
{
    SDL_Surface* surfaceImage = IMG_Load(filename);
    return surfaceImage;
}

SDL_Surface* ConvertTextToSurface(std::string text)
{
    SDL_Surface* surfaceText = SDL_CreateRGBSurface(0, pictureOnScreen.w,  pictureOnScreen.h, 32, rmask, gmask, bmask, amask);

    if(surfaceText == NULL)
    {
        SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
        return NULL;
    }

    if(pictureOnScreen.w * pictureOnScreen.h * 4 <= text.length())
    {
        std::cerr
            << "The Text has " << text.length() << " characters, but your selected size allows only " << pictureOnScreen.w * pictureOnScreen.h * 4 - 1 << " characters." << std::endl
            << "Set a bigger size or use the --autosize argument." << std::endl;
    }

    Uint32* pixels = (Uint32 *) surfaceText->pixels;

    for(int i = 0; i * 4 < text.length(); i++)
    {
        pixels[i] = SDL_MapRGBA(surfaceText->format, text[i * 4], text[i * 4 + 1], text[i * 4 + 2], text[i * 4 + 3]);
    }

    return surfaceText;

}

std::string ConvertSurfaceToText(SDL_Surface* surface)
{
    std::string text = "";
    Uint32* pixels = (Uint32 *) surface->pixels;

    unsigned char a, b, c, d;

    int i = 0;
    do
    {
        a = pixels[i] & rmask >> rshift;
        b = (pixels[i] & gmask) >> gshift;
        c = (pixels[i] & bmask) >> bshift;
        d = (pixels[i] & amask) >> ashift;

        if(a)
            text += a;
        if(b)
            text += b;
        if(c)
            text += c;
        if(d)
            text += d;
            
        i++;
    } while (a && b && c && d);
    
    return text;
}

int main(int argc, char *argv[])
{
    bool toPicture = false;
    bool toText = false;
    bool show = false;
    bool autosize = false;
    std::string inFile = "";
    std::string outFile = "";

    pictureOnScreen.h = 256;
    pictureOnScreen.w = 256;
    pictureOnScreen.x = 0;
    pictureOnScreen.y = 0;

    std::string text = "";
    SDL_Surface* surfaceImage = NULL;

    // search arguments
    for(int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if(arg == "-p" || arg == "--picture")
            toPicture = true;

        else if(arg == "-t" || arg == "--text")
            toText = true;

        else if(arg == "--show")
            show = true;

        else if(arg == "-i" || arg == "--input")
            if(i < argc - 1)
            {
                inFile = argv[++i];
            }
            else
            {
                std::cerr << arg << " requires one argument." << std::endl;
                return -1;
            }

        else if(arg == "-o" || arg == "--output")
            if(i < argc - 1)
            {
                outFile = argv[++i];
            }
            else
            {
                std::cerr << arg << " requires one argument." << std::endl;
                return -1;
            }

        else if(arg == "-x" || arg == "--width")
            if(i < argc - 1)
            {
                pictureOnScreen.w = atoi(argv[++i]);
            }
            else
            {
                std::cerr << arg << " requires one argument." << std::endl;
                return -1;
            }

        else if(arg == "-y" || arg == "--height")
            if(i < argc - 1)
            {
                pictureOnScreen.h = atoi(argv[++i]);
            }
            else
            {
                std::cerr << arg << " requires one argument." << std::endl;
                return -1;
            }

        else if(arg == "-a" || arg == "--autosize")
            {
                autosize = true;
            }

        else if (arg == "-h" || arg == "--help")
            {
                std::cout <<
                    "Usage: " << argv[0] << " [options]" << std::endl <<
                    "Options:" << std::endl <<
                    "  -h, --help\t\t\tshow this help text" << std::endl <<
                    "  -p, --picture\t\t\tconvert text to image" << std::endl <<
                    "  -t, --text\t\t\tconvert image to text" << std::endl <<
                    "  --show\t\t\tshow the image in a window" << std::endl <<
                    "  -i FILE, --input FILE\t\tinput file" << std::endl <<
                    "  -o FILE, --output FILE\toutput file" << std::endl <<
                    "  -x COUNT, --width COUNT\timage width" << std::endl <<
                    "  -y COUNT, --height COUNT\timage height" << std::endl <<
                    "  -a, --autosize\t\tauto-size image" << std::endl <<
                    std::endl;
                return 0;
            }

        else                                                                    // invalid argument
        {
            std::cerr << arg << " is not a valid argument." << std::endl;
            return -1;
        }    
    }

    if(inFile == "")
    {
        std::cerr << "No input file selected" << std::endl;
        return -1;
    }

    // initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        std::cerr << SDL_GetError() << std::endl;
        return -1;
    }


    if(toPicture)
    {
        // load text
        text = LoadText(inFile.c_str());

        if(autosize)
        {
            pictureOnScreen.h = sqrt(text.length() / 4) + 1;
            pictureOnScreen.w = sqrt(text.length() / 4) + 1;
        }

        // Convert to image
        surfaceImage = ConvertTextToSurface(text);

        // Save Image
        if(outFile != "")
            IMG_SavePNG(surfaceImage, outFile.c_str());
    }
    


    if(toText)
    {
        // load PNG
        surfaceImage = LoadPNG(inFile.c_str());

        // Convert to text
        text = ConvertSurfaceToText(surfaceImage);

        // Save Text
        if(outFile != "")
            SaveText(text, outFile.c_str());
        else
            std::cout << text << std::endl;
    }
    
    if(show)
        ShowPicture(surfaceImage);

    SDL_FreeSurface(surfaceImage);
    SDL_Quit();
    return 0;
}