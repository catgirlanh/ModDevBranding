
#include <Geode/Geode.hpp>
#include "ImageCache.hpp"


ImageCache* ImageCache::instance = nullptr;

ImageCache::ImageCache(){
    m_imageDict = CCDictionary::create();
}

void ImageCache::addImage(CCImage* image, std::string key){

    if(!image) return;
    m_imageDict->setObject(image, key);
}

CCImage* ImageCache::getImage(std::string key){
    return static_cast<CCImage*>(m_imageDict->objectForKey(key));
    
}