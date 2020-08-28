// standard headers:
#include <iostream>
#include <vector>
#include <exception>
#include <string>

// system services:
#include <unistd.h>

//// system services ///

void sleep_ms(unsigned long long ms)
{
    (void)usleep(ms * 1000);
}

//// util ////

class Exception : public std::exception
{
public:
    explicit Exception(const char *msg) : _msg(msg) { }
    explicit Exception(const std::string & msg) : _msg(msg) { }

    const char* what() const noexcept final {
        return _msg.c_str();
    }

protected:
    std::string _msg;
};

//// drawing functions ////

class MoveCursorHelper
{
public:
    MoveCursorHelper(unsigned row, unsigned col) : _row(row), _col(col) { }

    std::ostream& action(std::ostream& os) const {
        os << "\x1b[" << _row << ';' << _col << 'H';
        os.flush();
        return os;
    }

private:
    const unsigned _row;
    const unsigned _col;
};

std::ostream& operator<<(std::ostream& os, const MoveCursorHelper& h)
{
    return h.action(os);
}

std::ostream& reset_cursor(std::ostream & os)
{
    os << "\x1b[H";
    os.flush();
    return os;
}

MoveCursorHelper move_cursor(unsigned row, unsigned col)
{
    return MoveCursorHelper(row, col);
}

//// graphing logic ////

class Point
{
public:
    Point() : _x(0.0), _y(0.0) { }
    Point(double x, double y) : _x(x), _y(y) { }

    double x() const { return _x; }
    double &x() { return _x; }
    double y() const { return _y; }
    double &y() { return _y; }

private:
    double _x;
    double _y;
};

typedef unsigned char pixel_t;

class Frame
{
public:
    Frame();

    void init(unsigned width, unsigned height);
    unsigned width() const { return _width; }
    unsigned height() const { return _height; }

private:
    unsigned _width;
    unsigned _height;
    std::vector<pixel_t> _screen;
};

const static unsigned MAX_WIDTH = 1024;
const static unsigned MAX_HEIGHT = 1024;

Frame::Frame() : _width(0), _height(0)
{
}

void Frame::init(unsigned width, unsigned height)
{
    if (width > MAX_WIDTH) {
        throw Exception("Width is greater than the maxiumum allowed");
    }
    else if (height > MAX_HEIGHT) {
        throw Exception("Height is greater than the maxiumum allowed");
    }

    _width = width;
    _height = height;

    // init empty frame
    _screen.resize(_width * _height, 0);
}

std::ostream& operator<<(std::ostream& os, const Frame& f)
{
    return os;
}

int main()
{
    std::cout << move_cursor(4, 8);
    sleep_ms(1000);
    std::cout << reset_cursor;
    sleep_ms(1000);
    return 0;
}

