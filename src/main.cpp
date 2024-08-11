#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/web.hpp>
#include "ImageCache.hpp"

using namespace geode::prelude;
EventListener<web::WebTask> listener;

void onImageDownloadFinish(std::string dev, CCImage* image, FLAlertLayer* alert, bool newImage, CCSprite* spr = nullptr){

	geode::Loader::get()->queueInMainThread([dev, image, alert, newImage, spr](){
		if(newImage){
			ImageCache::get()->addImage(image, fmt::format("dev-{}", dev));
			image->release();
		}

		CCSprite* newSprite;

		if(!spr){
			CCTexture2D* texture = new CCTexture2D();
			if(texture->initWithImage(image)){
				newSprite = CCSprite::createWithTexture(texture);
			}
			texture->release();
		}
		else {
			newSprite = spr;
		}
		if(newSprite) {
			newSprite->setID("dev-watermark"_spr);
			newSprite->setScale(1/(4/CCDirector::sharedDirector()->getContentScaleFactor()));
			newSprite->setAnchorPoint({1, 0});
			newSprite->setOpacity(40);
			if(alert && alert->m_mainLayer){
				if(auto descriptionContainer = alert->m_mainLayer->getChildByIDRecursive("description-container")){
					if(auto textArea = descriptionContainer->getChildByID("textarea")){
						if(!textArea->getChildByID("dev-watermark"_spr)) {
							
							float textAreaWidth = textArea->getContentSize().width;
							float descriptionContainerWidth = descriptionContainer->getContentSize().width;
							float difference = descriptionContainerWidth - textAreaWidth;
							newSprite->setPositionX(textAreaWidth + difference/2);
							textArea->addChild(newSprite);
						}
						if(auto scrollLayer = textArea->getChildByID("ScrollLayer")){
							scrollLayer->setZOrder(1);
						}
					}
				}
			}
		}
		if(spr){
			spr->release();
		}
	});
}

$execute {

	new EventListener<EventFilter<ModPopupUIEvent>>(+[](ModPopupUIEvent* event) {

		FLAlertLayer* modPopup = nullptr;

		if(auto popup = event->getPopup()){
			modPopup = popup;
		}

		if(event->getModID() == "alphalaneous.moddevbranding"){

			auto path = Mod::get()->getSettingValue<std::filesystem::path>("preview-image");

			if(std::filesystem::exists(path)){

				if(auto sprite = CCSprite::create(path.string().c_str())) {
					sprite->retain();
					onImageDownloadFinish("", nullptr, modPopup, false, sprite);
				}

				return ListenerResult::Propagate;
			}
		}

		std::string lowerDev = "";

		std::optional modOpt = event->getMod();
		if(modOpt.has_value()){
			Mod* mod = modOpt.value();
			lowerDev = string::toLower(mod->getDeveloper());
		}
		else {
			//cheating to get dev username by ID until ModMetadata is exposed
			if(!event->getModID().empty()) {
				lowerDev = string::toLower(string::split(event->getModID(), ".").at(0));
			}
		}

		if(lowerDev.empty()) return ListenerResult::Propagate;

		if(CCImage* img = ImageCache::get()->getImage(fmt::format("dev-{}", lowerDev))){
			onImageDownloadFinish(lowerDev, img, modPopup, false);
			return ListenerResult::Propagate;
		}

		std::string url = fmt::format("https://raw.githubusercontent.com/Alphalaneous/ModDevBranding-Images/main/Images/{}.png", lowerDev);

		auto req = web::WebRequest();
		listener.bind([lowerDev, event, modPopup](web::WebTask::Event* e){
			if (auto res = e->getValue()){
				if (res->ok()) {
					auto data = res->data();
					std::thread imageThread = std::thread([data, lowerDev, event, modPopup](){
						CCImage* image = new CCImage();

						if(!image->initWithImageData(const_cast<uint8_t*>(data.data()), data.size())) return;
						
						onImageDownloadFinish(lowerDev, image, modPopup, true);

					});
					imageThread.detach();
				}
			}
		});
		auto downloadTask = req.get(url);
		listener.setFilter(downloadTask);
		
        return ListenerResult::Propagate;
    });
}