#include <sstream>
#include <glm/gtc/matrix_transform.hpp>

#include "PatternSource.h"

#include "defines.h"
#include "ImageShader.h"
#include "Resource.h"
#include "Primitives.h"
#include "Stream.h"
#include "Visitor.h"
#include "Log.h"

//    smpte (0) – SMPTE 100%% color bars
//    snow (1) – Random (television snow)
//    black (2) – 100%% Black
//    white (3) – 100%% White
//    red (4) – Red
//    green (5) – Green
//    blue (6) – Blue
//    checkers-1 (7) – Checkers 1px
//    checkers-2 (8) – Checkers 2px
//    checkers-4 (9) – Checkers 4px
//    checkers-8 (10) – Checkers 8px
//    circular (11) – Circular
//    blink (12) – Blink
//    smpte75 (13) – SMPTE 75%% color bars
//    zone-plate (14) – Zone plate
//    gamut (15) – Gamut checkers
//    chroma-zone-plate (16) – Chroma zone plate
//    solid-color (17) – Solid color
//    ball (18) – Moving ball
//    smpte100 (19) – SMPTE 100%% color bars
//    bar (20) – Bar
//    pinwheel (21) – Pinwheel
//    spokes (22) – Spokes
//    gradient (23) – Gradient
//    colors (24) – Colors
const char* pattern_internal_[25] = { "videotestsrc pattern=black",
                                      "videotestsrc pattern=white",
                                      "frei0r-src-test-pat-l",
                                      "videotestsrc pattern=gradient",
                                      "videotestsrc pattern=checkers-1 ! video/x-raw,format=GRAY8 ! videoconvert",
                                      "frei0r-src-test-pat-g",
                                      "videotestsrc pattern=circular",
                                      "videotestsrc pattern=pinwheel",
                                      "videotestsrc pattern=spokes",
                                      "videotestsrc pattern=red",
                                      "videotestsrc pattern=green",
                                      "videotestsrc pattern=blue",
                                      "videotestsrc pattern=smpte100",
                                      "frei0r-src-test-pat-c",
                                      "videotestsrc pattern=colors",
                                      "videotestsrc pattern=smpte",
                                      "videotestsrc pattern=snow",
                                      "videotestsrc pattern=blink",
                                      "videotestsrc pattern=zone-plate",
                                      "videotestsrc pattern=chroma-zone-plate",
                                      "videotestsrc pattern=bar horizontal-speed=5",
                                      "videotestsrc pattern=ball",
                                      "frei0r-src-ising0r",
                                      "videotestsrc pattern=black ! timeoverlay halignment=center valignment=center font-desc=\"Sans, 72\" ",
                                      "videotestsrc pattern=black ! clockoverlay halignment=center valignment=center font-desc=\"Sans, 72\" "
                                    };

std::vector<std::string> Pattern::pattern_types = { "100% Black",
                                         "100% White",
                                         "Gray bars",
                                         "Gradient",
                                         "Checkers 1x1 px",
                                         "Checkerboard",
                                         "Circles",
                                         "Pinwheel",
                                         "Spokes",
                                         "100% Red",
                                         "100% Green",
                                         "100% Blue",
                                         "Color bars",
                                         "Color Gradient",
                                         "Color grid",
                                         "SMPTE test pattern",
                                         "Television snow",
                                         "Blink",
                                         "Fresnel zone plate",
                                         "Chroma zone plate",
                                         "Moving bar",
                                         "Moving ball",
                                         "Blob",
                                         "Timer",
                                         "Clock"
                                       };

Pattern::Pattern(glm::ivec2 res) : Stream()
{

    width_ = res.x;
    height_ = res.y;
}

glm::ivec2 Pattern::resolution()
{
    return glm::ivec2( width_, height_);
}


void Pattern::open( uint pattern )
{
    type_ = CLAMP(pattern, 0, 25);
    std::string gstreamer_pattern = pattern_internal_[type_];

    // there is always a special case...
    switch(type_)
    {
    case 18:
    case 19:
    {
        std::ostringstream oss;
        oss << " kx2=" << (int)(aspectRatio() * 10.f) << " ky2=10 kt=4";
        gstreamer_pattern += oss.str(); // Zone plate
    }
        break;
    default:
        break;
    }

    // all patterns before index are single frames (not animated)
    single_frame_ = type_ < 15;

    // (private) open stream
    Stream::open(gstreamer_pattern);
}

PatternSource::PatternSource(glm::ivec2 resolution) : StreamSource()
{
    // create stream
    stream_ = (Stream *) new Pattern(resolution);

    // set icons
    overlays_[View::MIXING]->attach( new Symbol(Symbol::PATTERN, glm::vec3(0.8f, 0.8f, 0.01f)) );
    overlays_[View::LAYER]->attach( new Symbol(Symbol::PATTERN, glm::vec3(0.8f, 0.8f, 0.01f)) );
}

void PatternSource::setPattern(int id)
{
    Log::Notify("Creating Source with pattern '%s'", Pattern::pattern_types[id].c_str());

    pattern()->open( (uint) id );
    stream_->play(true);
}

void PatternSource::accept(Visitor& v)
{
    Source::accept(v);
    v.visit(*this);
}

Pattern *PatternSource::pattern() const
{
    return dynamic_cast<Pattern *>(stream_);
}


