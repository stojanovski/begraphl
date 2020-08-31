// standard headers:
#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <cassert>
#include <cmath>

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

class Range
{
public:
    Range() : _from(0.0), _to(0.0) { }
    Range(double f, double t) : _from(f), _to(t) {
        assert(t >= f);
    }

    double from() const { return _from; }
    double to() const { return _to; }
    double& from() { return _from; }
    double& to() { return _to; }
    double distance() const {
        return _to - _from;
    }

private:
    double _from;
    double _to;
};

class CoordinateSystem
{
public:
    CoordinateSystem(Frame& frame);
    void set_range(const Range& x_axis, double y_origin_offset = 0);
    const Range& x_axis_range() const {
        return _x_axis_range;
    }
    void draw_at(const Point& p);

private:
    Frame & _frame;
    Range _x_axis_range;
    Range _y_axis_range;
};

CoordinateSystem::CoordinateSystem(Frame& frame) :
    _frame(frame)
{
}

void CoordinateSystem::set_range(const Range& x_axis, double y_origin_offset)
{
    _x_axis_range = x_axis;
    const double half_distance = _x_axis_range.distance() / 2.0;
    const double y_to_x_frame_ratio = (double)_frame.height() /
                                      (double)_frame.width();
    _y_axis_range = Range(y_origin_offset - (half_distance * y_to_x_frame_ratio),
                          y_origin_offset + (half_distance * y_to_x_frame_ratio));
}

void CoordinateSystem::draw_at(const Point& p)
{
    // TODO(igor): implement
    (void)p;
}

class TestSinChart
{
public:
    TestSinChart();

    void init(const Range& x_axis_range,
              unsigned frame_width,
              unsigned frame_height);
    void run();

private:
    Frame _frame;
    CoordinateSystem _coor_sys;
};

TestSinChart::TestSinChart() : _coor_sys(_frame)
{
}

void TestSinChart::init(const Range& x_axis_range,
                        unsigned frame_width,
                        unsigned frame_height)
{
    _frame.init(frame_width, frame_height);
    _coor_sys.set_range(x_axis_range);
}

void TestSinChart::run()
{
    const Range& x_axis_range = _coor_sys.x_axis_range();
    const double start = x_axis_range.from();
    const double increment = x_axis_range.distance() / (double)_frame.width();

    for (unsigned i = 0; i < _frame.width(); ++i) {
        const double x = start + ((double)i * increment);
        const double y = sin(x);
        _coor_sys.draw_at(Point{x, y});
    }
}

int main()
{
    std::cout << move_cursor(4, 8);
    sleep_ms(1000);
    std::cout << reset_cursor;
    sleep_ms(1000);
    return 0;
}

