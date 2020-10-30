// standard headers:
#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <cassert>
#include <cmath>   // sin()

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

std::ostream& clear_screen(std::ostream & os)
{
    os << "\x1b[2J";
    os.flush();
    return os;
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

const pixel_t PIXEL_T_EMPTY = 0;
const pixel_t PIXEL_T_FILLED = 1;

class Frame
{
public:
    Frame(unsigned width, unsigned height);

    unsigned width() const { return _width; }
    unsigned height() const { return _height; }
    void draw(unsigned row, unsigned col, pixel_t pix);

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
        throw Exception("Frame width cannot be greater than the maxiumum allowed");
    }
    else if (_height > MAX_HEIGHT) {
        throw Exception("Frame height cannot be greater than the maxiumum allowed");
    }

    // init empty frame
    _screen.resize(_width * _height, 0);
}

void Frame::draw(unsigned row, unsigned col, pixel_t pix)
{
    if (row >= _height || col >= _width) {
        // out of bounds: ignore since we can't fit it in the frame
        return;
    }

    std::cout << move_cursor(row, col) << '+';
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
    StepRange(Range range, size_t steps);

    size_t steps() const {
        return _steps;
    }

private:
    const size_t _steps;
};

StepRange::StepRange(Range range, size_t steps) :
    Range(range), _steps(steps)
{
    if (distance() == 0.0 || _steps < 2) {
        throw Exception("StepRange must be of non-zero distance and have at least 2 steps");
    }
}

class CoordinateSystem
{
public:
    class Iterator {
    public:
        bool next(double *x) {
            if (_cur_step < _steps) {
                *x = _from + (_cur_step++ * (_dist / (_steps - 1)));
                return true;
            }

            return false;
        }

        void set(double value) {
            assert(_cur_step <= _steps);

            const Range& yr = _coor._y_axis_range;
            if (value < yr.from() || value > yr.to()) {
                // value is out of range: ignore
                return;
            }

            // convert "value" to a proper Frame value
            const double ratio = (value - yr.from()) / yr.distance();
            Frame& frame = _coor._frame;
            const double col = frame.height() - (ratio * frame.height());

            frame.draw((unsigned)col, (unsigned)_cur_step, PIXEL_T_FILLED);
        }

    private:
        Iterator(CoordinateSystem& coor, double from, double dist, size_t steps) :
            _coor(coor), _from(from), _dist(dist), _steps(steps), _cur_step(0)
        {
            assert(_dist > 0.0);
            assert(_steps > 1);
        }

        CoordinateSystem& _coor;
        double _from;
        double _dist;
        size_t _steps;
        size_t _cur_step;

        friend class CoordinateSystem;
    };

    CoordinateSystem(Frame& frame, const Range& x_axis, double y_origin_offset = 0.0);
    const StepRange& x_axis_range() const {
        return _x_axis_range;
    }
    Iterator iterator();

private:
    Frame& _frame;
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

CoordinateSystem::Iterator CoordinateSystem::iterator()
{
    return Iterator(*this, _x_axis_range.from(), _x_axis_range.distance(), _x_axis_range.steps());
}

class FuncChart
{
public:
    FuncChart(const Range& x_axis_range,
              unsigned frame_width,
              unsigned frame_height);

    void run();

private:
    Frame _frame;
    CoordinateSystem _coor_sys;
    const unsigned _frame_height;
};

FuncChart::FuncChart(const Range& x_axis_range,
                     unsigned frame_width,
                     unsigned frame_height) :
    _frame(frame_width, frame_height),
    _coor_sys(_frame, x_axis_range),
    _frame_height(frame_height)
{
}

void FuncChart::run()
{
    std::cout << clear_screen;

    CoordinateSystem::Iterator it = _coor_sys.iterator();
    double x;
    while (it.next(&x)) {
        const double y = sin(x);
        it.set(y);
    }

    std::cout << move_cursor(_frame_height + 1, 0);
}

static void test_move_cursor()
{
    std::cout << clear_screen;
    for (unsigned i = 1; i <= 10; ++i) {
        std::cout << move_cursor(i, i) << '+';
        sleep_ms(250);
    }
}

static void test_FuncChart()
{
    FuncChart func_chart(Range(-6.0, 6.0), 100, 40);
    func_chart.run();
}

int main()
{
    test_FuncChart();
    //test_move_cursor();
    return 0;
}

