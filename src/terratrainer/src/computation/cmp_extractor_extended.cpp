#include "cmp_extractor_extended.h"
#include "yaml.hpp"

using namespace cv_extraction;

CMPFeatureExtractorExt::CMPFeatureExtractorExt() :
    max_octave_(0),
    color_extension_(false),
    combine_descriptors_(false)
{
}

void CMPFeatureExtractorExt::extractToYAML(YAML::Emitter  &emitter, const cv::Mat &img, std::vector<cv_roi::TerraROI> rois)
{
    for(std::vector<cv_roi::TerraROI>::iterator it = rois.begin() ; it != rois.end() ; it++) {
        cv::Mat desc;
        extract(img, it->roi.rect, key_, max_octave_, color_extension_, combine_descriptors_, desc);

        if(type_ == ExtractorParams::SURF || type_ == ExtractorParams::SIFT) {
            CMPYAML::writeDescriptorRows<float, float>(desc, it->id.id, emitter);
        } else  {
            CMPYAML::writeDescriptorRows<uchar, int>(desc, it->id.id, emitter);
        }
    }

}

void CMPFeatureExtractorExt::extractToYAML(YAML::Emitter  &emitter, const cv::Mat &img, std::vector<cv_roi::TerraROI> rois,
                                      CMPStatePublisher::Ptr state)
{
    std::pair<int,int> counts(0, rois.size());

    for(std::vector<cv_roi::TerraROI>::iterator it = rois.begin() ; it != rois.end() ; it++, counts.first++) {
        cv::Mat desc;
        extract(img, it->roi.rect, key_, max_octave_, color_extension_, combine_descriptors_, desc);

        if(type_ == ExtractorParams::SURF || type_ == ExtractorParams::SIFT) {
            CMPYAML::writeDescriptorRows<float, float>(desc, it->id.id, emitter);
        } else  {
            CMPYAML::writeDescriptorRows<uchar, int>(desc, it->id.id, emitter);
        }

        state->publish(counts);
    }

}

void CMPFeatureExtractorExt::setParams(const ParamsORB &params)
{
    FeatureExtractor::set(getExtractor(params));
    setCommonParameters(params);
}

void CMPFeatureExtractorExt::setParams(const ParamsSURF &params)
{
    FeatureExtractor::set(getExtractor(params));
    setCommonParameters(params);
}

void CMPFeatureExtractorExt::setParams(const ParamsSIFT &params)
{
    FeatureExtractor::set(getExtractor(params));
    setCommonParameters(params);
}

void CMPFeatureExtractorExt::setParams(const ParamsBRISK &params)
{
    FeatureExtractor::set(getExtractor(params));
    setCommonParameters(params);
}

void CMPFeatureExtractorExt::setParams(const ParamsBRIEF &params)
{
    FeatureExtractor::set(getExtractor(params));
    setCommonParameters(params);
}

void CMPFeatureExtractorExt::setParams(const ParamsFREAK &params)
{
    FeatureExtractor::set(getExtractor(params));
    setCommonParameters(params);
}

void CMPFeatureExtractorExt::setKeyPointParams(const KeypointParams &key)
{
    key_ = key;
}

void CMPFeatureExtractorExt::setCommonParameters(const cv_extraction::ExtractorParams &params)
{
    max_octave_             = params.octaves;
    type_                   = params.type;
    color_extension_        = params.color_extension;
    combine_descriptors_    = params.combine_descriptors;
}


CMPPatternExtractorExt::CMPPatternExtractorExt() :
    color_extension_(false),
    combine_descriptors_(true)
{
}

void CMPPatternExtractorExt::extractToYAML(YAML::Emitter  &emitter, const cv::Mat &img, std::vector<cv_roi::TerraROI> &rois)
{
    for(std::vector<cv_roi::TerraROI>::iterator it = rois.begin() ; it != rois.end() ; it++) {
        /// CALCULATION
        cv::Mat     roi(img, it->roi.rect);

        /// COLOR EXTENSION
        cv::Mat desc;
        extract(roi, color_extension_, combine_descriptors_, desc);
        CMPYAML::writeDescriptorRows<int, int>(desc, it->id.id, emitter);
    }
}

void CMPPatternExtractorExt::extractToYAML(YAML::Emitter &emitter, const cv::Mat &img, std::vector<cv_roi::TerraROI> &rois,
                   CMPStatePublisher::Ptr state)
{
    std::pair<int,int> counts(0, rois.size());

    for(std::vector<cv_roi::TerraROI>::iterator it = rois.begin() ; it != rois.end() ; it++, counts.first++) {
        /// CALCULATION
        cv::Mat     roi(img, it->roi.rect);

        /// COLOR EXTENSION
        cv::Mat desc;
        extract(roi, color_extension_, combine_descriptors_, desc);
        CMPYAML::writeDescriptorRows<int, int>(desc, it->id.id, emitter);
        state->publish(counts);
    }
}


void CMPPatternExtractorExt::setParams(const cv_extraction::ParamsLBP &params)
{
    PatternExtractor::set(new cv_local_patterns::LBP);
    k = 0;
    color_extension_ = params.color_extension;
    combine_descriptors_ = params.combine_descriptors;
}

void CMPPatternExtractorExt::setParams(const cv_extraction::ParamsLTP &params)
{
    PatternExtractor::set(new cv_local_patterns::LTP);
    k                    = params.k;
    color_extension_     = params.color_extension;
    combine_descriptors_ = params.combine_descriptors;
}

