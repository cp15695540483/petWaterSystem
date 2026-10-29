/**
 * @file filters.h
 * @brief 滑动平均滤波器 - 用于水位、TDS 等模拟量去噪
 */

#ifndef FILTERS_H
#define FILTERS_H

/**
 * @brief 滑动平均滤波器类
 */
class MovingAverageFilter {
public:
    /**
     * @param size 窗口大小（建议 8~16）
     */
    MovingAverageFilter(int size);
    ~MovingAverageFilter();

    /**
     * @brief 输入新采样值，返回滤波后的结果
     */
    float update(float value);

    /**
     * @brief 重置滤波器（清空历史数据）
     */
    void reset();

    /**
     * @brief 获取当前滤波输出（不更新）
     */
    float getValue() const { return _out; }

    /**
     * @brief 是否已填满窗口（滤波稳定）
     */
    bool isReady() const { return _count >= _size; }

private:
    float* _buf;
    int    _size;
    int    _head;
    int    _count;
    float  _sum;
    float  _out;
};

#endif /* FILTERS_H */
