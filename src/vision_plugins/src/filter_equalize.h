#ifndef FILTER_EUQALIZE_H
#define FILTER_EQUALIZE_H

/// COMPONENT
#include <vision_evaluator/filter.h>

namespace vision_plugins {
/**
 * @brief The equalization plugin represents the cv function to euqalize an image using histograms.
 */
class Equalize : public vision_evaluator::Filter
{
    Q_OBJECT

public:
    /**
     * @brief Equalize default constructor.
     */
    Equalize();
    /**
     * @brief ~Equalize destructor.
     */
    virtual ~Equalize();

    /**
     * @brief See base class definition.
     */
    virtual void insert(QBoxLayout *parent);
    /**
     * @brief See base class definition.
     */
    virtual void filter(cv::Mat &img, cv::Mat &mask);
protected:
    virtual bool usesMask();

};
}



#endif // FILTER_EQUALIZE_H