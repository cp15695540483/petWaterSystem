/**
 * @file filters.cpp
 * @brief 滑动平均滤波器实现
 */

#include "filters.h"

MovingAverageFilter::MovingAverageFilter(int size) {
    _size  = (size > 1) ? size : 2;
    _buf   = new float[_size];
    _head  = 0;
    _count = 0;
    _sum   = 0;
    _out   = 0;
    for (int i = 0; i < _size; i++) _buf[i] = 0;
}

MovingAverageFilter::~MovingAverageFilter() {
    delete[] _buf;
}

float MovingAverageFilter::update(float value) {
    if (_count < _size) {
        _buf[_count] = value;
        _sum += value;
        _count++;
    } else {
        _sum -= _buf[_head];
        _buf[_head] = value;
        _sum += value;
        _head = (_head + 1) % _size;
    }
    _out = _sum / _count;
    return _out;
}

void MovingAverageFilter::reset() {
    _head  = 0;
    _count = 0;
    _sum   = 0;
    _out   = 0;
    for (int i = 0; i < _size; i++) _buf[i] = 0;
}
