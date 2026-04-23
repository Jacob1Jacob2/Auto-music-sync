#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

std::vector<int> slowSpeed;
std::vector<int> normalSpeed;
std::vector<int> fastSpeed;
std::vector<int> fasterSpeed;
std::vector<int> fastestSpeed;

void cleanVector(std::vector<int>& vector) {
    std::sort(vector.begin(), vector.end());
    auto uniquePositions = std::unique(vector.begin(), vector.end());
    vector.erase(uniquePositions, vector.end());
}

class $modify(ResetMusicLayer, PlayLayer) {
    struct Fields {
        float pitch = 1.0f;
        float supposedSpeed = 0.0f;
        bool needingOfPitch = false;
        int pixelBuffer = 0;
    };

    void resetMusic() {
        m_fields->needingOfPitch = false;
        auto fmod = FMODAudioEngine::sharedEngine();
        if (fmod && fmod->m_system) {
            FMOD::ChannelGroup* mGroup = nullptr;
            fmod->m_system->getMasterChannelGroup(&mGroup);
            if (mGroup) mGroup->setPitch(1.0f);
        }
    }

    void quickRestart() {
        m_fields->supposedSpeed = 0.0f;
        auto fmod = FMODAudioEngine::sharedEngine();
        if (fmod && fmod->m_system) {
            FMOD::ChannelGroup* mGroup = nullptr;
            fmod->m_system->getMasterChannelGroup(&mGroup);
            m_fields->needingOfPitch = false;
            if (mGroup) mGroup->setPitch(1.0f);
        }
        m_fields->needingOfPitch = true;
    }

    bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects) {
        // Clear vectors BEFORE init populates them
        slowSpeed.clear(); 
        normalSpeed.clear(); 
        fastSpeed.clear(); 
        fasterSpeed.clear(); 
        fastestSpeed.clear();

        // Safely fetch the setting
        try {
            m_fields->pixelBuffer = (int)Mod::get()->getSettingValue<int64_t>("pixelBuffer");
        } catch(...) {
            m_fields->pixelBuffer = 0;
        }

        // Running the original init will trigger addObject for every item in the level
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        m_fields->supposedSpeed = 0.0f;
        this->quickRestart(); 
        
        // Clean the vectors now that they are fully populated
        cleanVector(slowSpeed); 
        cleanVector(normalSpeed); 
        cleanVector(fastSpeed); 
        cleanVector(fasterSpeed); 
        cleanVector(fastestSpeed);
        
        return true;
    }

    // NEW ENGINE-NATIVE HOOK: Intercepts objects as they spawn to find speed portals natively
    void addObject(GameObject* obj) {
        PlayLayer::addObject(obj);
        
        if (!obj) return;

        int id = obj->m_objectID;
        // If it's a speed portal, grab its position
        if (id == 200 || id == 201 || id == 202 || id == 203 || id == 1334) {
            float x = obj->getPositionX();
            int pb = m_fields->pixelBuffer;
            
            if (id == 200) slowSpeed.push_back((int)x - pb);
            else if (id == 201) normalSpeed.push_back((int)x - pb);
            else if (id == 202) fastSpeed.push_back((int)x - pb);
            else if (id == 203) fasterSpeed.push_back((int)x - pb);
            else if (id == 1334) fastestSpeed.push_back((int)x - pb);
        }
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        
        if (m_fields->supposedSpeed == 0.0f) m_fields->supposedSpeed = m_player1->m_playerSpeed; 
        
        if (m_fields->needingOfPitch) {
            float currentX = floor(m_player1->getPositionX());
            
            for (int i = 0; i < slowSpeed.size(); i++) {
                if (std::abs(currentX - slowSpeed[i]) <= 5) {
                    m_fields->supposedSpeed = 0.7f;
                    m_fields->pitch = m_player1->m_playerSpeed / 0.7f;
                }
            }
            for (int i = 0; i < normalSpeed.size(); i++) {
                if (std::abs(currentX - normalSpeed[i]) <= 5) {
                    m_fields->supposedSpeed = 0.9f;
                    m_fields->pitch = m_player1->m_playerSpeed / 0.9f;
                }
            }
            for (int i = 0; i < fastSpeed.size(); i++) {
                if (std::abs(currentX - fastSpeed[i]) <= 5) {
                    m_fields->supposedSpeed = 1.1f;
                    m_fields->pitch = m_player1->m_playerSpeed / 1.1f;
                }
            }
            for (int i = 0; i < fasterSpeed.size(); i++) {
                if (std::abs(currentX - fasterSpeed[i]) <= 5) {
                    m_fields->supposedSpeed = 1.3f;
                    m_fields->pitch = m_player1->m_playerSpeed / 1.3f;
                }
            }
            for (int i = 0; i < fastestSpeed.size(); i++) {
                if (std::abs(currentX - fastestSpeed[i]) <= 5) {
                    m_fields->supposedSpeed = 1.6f;
                    m_fields->pitch = m_player1->m_playerSpeed / 1.6f;
                }
            }
            
            if (!m_player1->m_isPlatformer) {
                auto fmod = FMODAudioEngine::sharedEngine();
                if (fmod && fmod->m_system) {
                    FMOD::ChannelGroup* mGroup = nullptr;
                    fmod->m_system->getMasterChannelGroup(&mGroup);
                    
                    if (mGroup) {
                        if (m_player1->m_playerSpeed != m_fields->supposedSpeed) {
                            mGroup->setPitch(m_fields->pitch);
                        }
                        else if (m_player1->m_playerSpeed == m_fields->supposedSpeed) {
                            mGroup->setPitch(1.0f);
                        }
                    }
                }
            }
        }
    }
    
    void updateAttempts() {
        PlayLayer::updateAttempts();
        this->resetMusic();
    }

    void onExit() {
        PlayLayer::onExit();
        this->resetMusic();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        this->resetMusic();
        this->quickRestart();
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        this->resetMusic();
    }
};