#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/BoomScrollLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>

using namespace geode::prelude;

int currentPage = 0;
bool bug = false;

// almost all of the code of this function is from kittenchilly, saved me A LOT of time
void updateFrames(PlayerObject* player) {
	if (!player) return;
    
    auto gameManager = GameManager::sharedState();
    auto PL = PlayLayer::get();
    if (!gameManager) return;

    if (player->m_isShip)
    {
        if (player->m_isPlatformer)
            player->updatePlayerJetpackFrame(gameManager->getPlayerJetpack());
        else
            player->updatePlayerShipFrame(gameManager->getPlayerShip());

        player->updatePlayerFrame(gameManager->getPlayerFrame());
    }
    else if (player->m_isBall)
        player->updatePlayerRollFrame(gameManager->getPlayerBall());
    else if (player->m_isBird)
    {
        player->updatePlayerBirdFrame(gameManager->getPlayerBird());
        player->updatePlayerFrame(gameManager->getPlayerFrame());
    }
    else if (player->m_isDart)
        player->updatePlayerDartFrame(gameManager->getPlayerDart());
    else if (player->m_isSwing)
        player->updatePlayerSwingFrame(gameManager->getPlayerSwing());
    else
        player->updatePlayerFrame(gameManager->getPlayerFrame());

    player->updatePlayerRobotFrame(gameManager->getPlayerRobot());
    player->updatePlayerSpiderFrame(gameManager->getPlayerSpider());

    int color1 = gameManager->getPlayerColor();
    int color2 = gameManager->getPlayerColor2();
    int glowColor = gameManager->getPlayerGlowColor();

    PL->m_player1->setColor(gameManager->colorForIdx(color1));
    PL->m_player2->setColor(gameManager->colorForIdx(color2));

    PL->m_player1->setSecondColor(gameManager->colorForIdx(color2));
    PL->m_player2->setSecondColor(gameManager->colorForIdx(color1));

    if (gameManager->getPlayerGlow())
    {
        PL->m_player1->enableCustomGlowColor(gameManager->colorForIdx(glowColor));
        PL->m_player2->enableCustomGlowColor(gameManager->colorForIdx(color1));
    }
    else
    {
        PL->m_player1->disableCustomGlowColor();
        PL->m_player2->disableCustomGlowColor();
    }
    player->updateGlowColor();
    player->updatePlayerGlow();
    player->m_playerGroundParticles->pauseSchedulerAndActions();
    player->m_vehicleGroundParticles->pauseSchedulerAndActions();
    player->m_playerStreak = gameManager->getPlayerStreak();

    gameManager->loadDeathEffect(gameManager->getPlayerDeathEffect());
}

void returnFix() { // without all of this it always goes to MenuLayer so yeah
    auto playLayer = GameManager::sharedState()->getPlayLayer();
    if (!playLayer) return;
        
    auto level = playLayer->m_level;
    if (!level) return;
        
    if (level->m_levelType == GJLevelType::Editor) {
        auto scene = CCScene::create();
        scene->addChild(EditLevelLayer::create(level));
        CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));
    } else if (level->m_levelType == GJLevelType::Local) {
		if (level->m_levelID >= 5001 && level->m_levelID <= 5004) {
			auto scene = CCScene::create();
        	scene->addChild(LevelAreaInnerLayer::create(true));
        	CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));
		} else if (level->m_levelID == 3001) {
			auto scene = CCScene::create();
        	scene->addChild(SecretLayer2::create());
        	CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));
		} else {
            auto scene = CCScene::create();
            scene->addChild(LevelSelectLayer::create(currentPage));
            CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));
		}
    } else {
		auto scene = CCScene::create();
        scene->addChild(LevelInfoLayer::create(level, false));
        CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));
	}
}

class $modify(PauseLayer) {
	static PauseLayer* create(bool p0) {
        auto ret = PauseLayer::create(p0);

		bug = true;
        
        auto menu = ret->getChildByID("right-button-menu");
        auto str = CCSprite::create("garage.png"_spr);
        auto btn = CCMenuItemSpriteExtra::create(
            str,
            ret,
            menu_selector(LevelInfoLayer::onGarage)
        );
        
        btn->setPosition({20.0f, 226.875f});
        btn->m_baseScale = 0.8f;
        btn->setScale(0.8f);
		btn->setID("nosu.icon-changer/icon-changer-button"_spr);
        menu->addChild(btn);
        
        return ret;
    }

	void onQuit(CCObject* sender) {
        returnFix();
	}
};

class $modify(BoomScrollLayer) {
	void updateDots(float p0) {
		BoomScrollLayer::updateDots(p0);

		if (!bug) {
			currentPage = m_page; // the BSL on the garage was messing this up so yeah
		}
	}
};

class $modify(LevelSelectLayer) {
	bool init(int page) {
		if (!LevelSelectLayer::init(page)) return false;

		bug = false;

		return true;
	}
};

class $modify(GJBaseGameLayer) { // the game was unpausing itself when getting out the garage so yeah
    void update(float dt) {
        auto PL = PlayLayer::get();
		if (!PL || !PL->m_isPaused) {
            GJBaseGameLayer::update(dt);
            m_player1->m_playerGroundParticles->resumeSchedulerAndActions(); // we resume the particles
            m_player2->m_playerGroundParticles->resumeSchedulerAndActions(); // bc it doesnt by itself for some reason
            m_player1->m_vehicleGroundParticles->resumeSchedulerAndActions();
            m_player2->m_vehicleGroundParticles->resumeSchedulerAndActions();
    	} else {
			updateFrames(m_player1);
            updateFrames(m_player2); 
		}	
	}
};

class $modify(EditorPauseLayer) { //this two werent working with the function so yeah
	void onExitNoSave(CCObject* sender) {
		auto level = LevelEditorLayer::get()->m_level;
        
        auto scene = CCScene::create();
        scene->addChild(EditLevelLayer::create(level));
        CCDirector::sharedDirector()->replaceScene(
            CCTransitionFade::create(0.5f, scene)
        );
	}

	void onSaveAndExit(CCObject* sender) {
        EditorPauseLayer::saveLevel();
        
        auto level = LevelEditorLayer::get()->m_level;
        
        auto scene = CCScene::create();
        scene->addChild(EditLevelLayer::create(level));
        CCDirector::sharedDirector()->replaceScene(
            CCTransitionFade::create(0.5f, scene)
        );
	}
};

class $modify(EndLevelLayer) {
	void onMenu(CCObject* sender) {
		returnFix();
	}
};