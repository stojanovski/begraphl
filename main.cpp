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
    Frame(unsigned width, unsigned height);

    unsigned width() const { return _width; }
    unsigned height() const { return _height; }

private:
    const unsigned _width;
    const unsigned _height;
    std::vector<pixel_t> _screen;
};

const static unsigned MAX_WIDTH = 1024;
const static unsigned MAX_HEIGHT = 1024;

Frame::Frame(unsigned width, unsigned height) :
    _width(width), _height(height)
{
    if (_width > MAX_WIDTH) {
        throw Exception("Width is greater than the maxiumum allowed");
    }
    else if (_height > MAX_HEIGHT) {
        throw Exception("Height is greater than the maxiumum allowed");
    }

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

class StepRange : public Range
{
public:
    class Iterator {
    public:
        Iterator() : _steps(0), _cur_step(0) { }

        bool next(double *point) {
            if (_cur_step < _steps) {
                *point = _from + (_cur_step++ * (_dist * (_steps - 1)));
                return true;
            }

            return false;
        }

    private:
        Iterator(double from, double dist, size_t steps) :
            _from(from), _dist(dist), _steps(steps), _cur_step(0)
        {
            assert(_dist > 0.0);
            assert(_steps > 1);
        }

        double _from;
        double _dist;
        size_t _steps;
        size_t _cur_step;

        friend class StepRange;
    };

    StepRange(Range range, size_t steps);

    Iterator iterator() const;

private:
    const size_t _steps;
};

StepRange::StepRange(Range range, size_t steps) :
    Range(range), _steps(steps)
{
    if (distance() == 0.0 || _steps < 2) {
        throw Exception("StepRange must be of non-zero distance and have multiple steps");
    }
}

StepRange::Iterator StepRange::iterator() const
{
    return Iterator(from(), distance(), _steps);
}

class CoordinateSystem
{
public:
    CoordinateSystem(Frame& frame, const Range& x_axis, double y_origin_offset = 0.0);
    const Range& x_axis_range() const {
        return _x_axis_range;
    }
    void set_point(const Point& p);

private:
    const Frame& _frame;
    const StepRange _x_axis_range;
    Range _y_axis_range;
};

CoordinateSystem::CoordinateSystem(Frame& frame, const Range& x_axis, double y_origin_offset) :
    _frame(frame), _x_axis_range(x_axis, frame.width())
{
    if (_frame.width() < 2) {
        throw Exception("Frame must be at least 2 units wide");
    }

    const double half_distance = _x_axis_range.distance() / 2.0;
    const double y_to_x_frame_ratio = (double)_frame.height() /
                                      (double)_frame.width();
    _y_axis_range = Range(y_origin_offset - (half_distance * y_to_x_frame_ratio),
                          y_origin_offset + (half_distance * y_to_x_frame_ratio));
}

void CoordinateSystem::set_point(const Point& p)
{
    (void)p;
}

class FuncChart
{
public:
    FuncChart(const Range& x_axis_range,
              unsigned frame_width,
              unsigned frame_height);

    void init(const Range& x_axis_range,
              unsigned frame_width,
              unsigned frame_height);
    void run();

private:
    Frame _frame;
    CoordinateSystem _coor_sys;
};

FuncChart::FuncChart(const Range& x_axis_range,
                     unsigned frame_width,
                     unsigned frame_height) :
    _frame(frame_width, frame_height),
    _coor_sys(_frame, x_axis_range)
{
}

void FuncChart::run()
{
    const Range& x_axis_range = _coor_sys.x_axis_range();
    const double start = x_axis_range.from();
    const double increment = x_axis_range.distance() / (double)_frame.width();

    for (unsigned i = 0; i < _frame.width(); ++i) {
        const double x = start + ((double)i * increment);
        const double y = sin(x);
        _coor_sys.set_point(Point{x, y});
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

