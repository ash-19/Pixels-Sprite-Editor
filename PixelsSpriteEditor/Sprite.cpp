#include "Sprite.h"

Sprite::Sprite(int w, int h, int s, QString file)
{
    width = w;
    height = h;
    fps = s;
    filename = file;
    frames.append(new Frame());

}

Sprite::Sprite(){
    width = 32;
    height = 32;
    fps = 30;
    filename = "sprite1.ssp";
    frames.append(new Frame());

}

Sprite::Sprite(const Sprite &other){
    this->width = other.width;
    this->height = other.height;
    this->fps = other.fps;
    this->filename = other.filename;
    QVector<Frame*> temp;
    this->frames = temp;

    for (int i = 0; i < other.frames.length(); i++){
        this->frames.append(other.frames[i]);
    }

}

Sprite& Sprite::operator=(Sprite other){
    std::swap(this->width, other.width);
    std::swap(this->height, other.height);
    std::swap(this->fps, other.fps);
    std::swap(this->filename, other.filename);
    std::swap(this->frames, other.frames);

    return *this;


}

Sprite::~Sprite()
{
    //delete timer;
}

const QVector<Frame*> Sprite::getFrames(){
    return this->frames;
}

void Sprite::addFrame()
{
    frames.append(new Frame());
}

Frame& Sprite::getFrame(int index)
{
    return *(frames[index]);
}

void Sprite::setFps(int speed)
{
    fps = speed;
}

int Sprite::getFps()
{
    return fps;
}

void Sprite::loadFile(QString file)
{
    frames.clear();
    filename = file;

    // Open file
    // Read file header and initialize variables
    // Parse lines and create frames
    // Close file
}

void Sprite::saveFile()
{
    // Open file
    // Save variables to header

    for(auto i = frames.begin(); i != frames.end(); i++)
    {
        // Convert to image
        // Get RGBA from each pixel
        // Write frame to file
    }

    // Close file
}

void Sprite::exportToGif(QString file)
{
    // Call to GifExport class
}


