// Pinball_Table_ModeMain.cpp:  PBTBL_MAIN mode implementation
//   Handles the primary gameplay screen including scores, character status,
//   resource icons, the mode system (normal play, multiball), and all
//   supporting render functions.

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"
#include "PBDevice.h"

// ========================================================================
// PBTBL_MAIN: Load Function
// ========================================================================

bool PBEngine::pbeLoadMainScreen(){
    
    if (m_mainScreenLoaded) return (true);

    // Load the main screen background
    m_PBTBLMainScreenBGId = gfxLoadSprite("MainScreenBG", "src/user/resources/textures/MainScreenBG.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLMainScreenBGId, 255, 255, 255, 255);
    
    // Load the 256 sprites
    m_PBTBLCharacterCircle256Id = gfxLoadSprite("CharacterCircle256", "src/user/resources/textures/CharacterCircle256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLCharacterCircle256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLCharacterCircle256Id, 0.6, false);
    
    m_PBTBLDungeon256Id = gfxLoadSprite("Dungeon256", "src/user/resources/textures/Dungeon256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLDungeon256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLDungeon256Id, 0.42, false);

    // Eye-blink overlay: solid black 30x30, no texture
    m_PBTBLDungeonEyesBlinkId = gfxLoadSprite("DungeonEyesBlink", "", GFX_NONE, GFX_NOMAP, GFX_UPPERLEFT, true, false);
    gfxSetWH(m_PBTBLDungeonEyesBlinkId, 30, 30);
    gfxSetColor(m_PBTBLDungeonEyesBlinkId, 3, 7, 22, 255);
    m_dungeonBlinkNextTick = (rand() % 14001) + 1000;  // Initial random delay of 1-15s
    
    m_PBTBLShield256Id = gfxLoadSprite("Shield256", "src/user/resources/textures/Shield256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLShield256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLShield256Id, 0.42, false);
    
    m_PBTBLSword256Id = gfxLoadSprite("Sword256", "src/user/resources/textures/Sword256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLSword256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLSword256Id, 0.42, false);
    
    m_PBTBLTreasure256Id = gfxLoadSprite("Treasure256", "src/user/resources/textures/Treasure256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLTreasure256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLTreasure256Id, 0.42, false);
    
    // Load the headshot sprites
    m_PBTBLArcherHeadshot256Id = gfxLoadSprite("ArcherHeadshot256", "src/user/resources/textures/ArcherHeadshot256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLArcherHeadshot256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLArcherHeadshot256Id, 0.5, false);
    
    m_PBTBLKnightHeadshot256Id = gfxLoadSprite("KnightHeadshot256", "src/user/resources/textures/KnightHeadshot256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLKnightHeadshot256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLKnightHeadshot256Id, 0.5, false);
    
    m_PBTBLWolfHeadshot256Id = gfxLoadSprite("WolfHeadshot256", "src/user/resources/textures/WolfHeadshot256.png", GFX_PNG, GFX_NOMAP, GFX_UPPERLEFT, true, true);
    gfxSetColor(m_PBTBLWolfHeadshot256Id, 255, 255, 255, 255);
    gfxSetScaleFactor(m_PBTBLWolfHeadshot256Id, 0.5, false);
    
    // Start the main screen music
    m_soundSystem.pbsPlayMusic(SOUNDMAINTHEME);

    m_mainScreenLoaded = true;
    return (true);
}

// ========================================================================
// PBTBL_MAIN: Render Functions
// ========================================================================

// Renders the base main screen elements that are always present
// (background, player scores, status icons, status text)
bool PBEngine::pbeRenderMainScreenBase(unsigned long currentTick, unsigned long lastTick){
    
    if (!pbeLoadMainScreen()) {
        pbeSendConsole("ERROR: Failed to load main screen resources");
        return (false);
    }

    // Clear to black background
    gfxClear(0.0f, 0.0f, 0.0f, 1.0f, false);
     
    // Render all player scores (skip main score during ball saved, inn open, and key obtained screens)
    {
        PBTBLMainScreenState curState = static_cast<PBTBLMainScreenState>(pbeGetCurrentSubScreenState());
        bool hideMain = (curState == PBTBLMainScreenState::MAIN_BALLSAVED ||
                         curState == PBTBLMainScreenState::MAIN_INN_OPEN ||
                         curState == PBTBLMainScreenState::MAIN_KEY_OBTAINED);
        if (!hideMain) {
            pbeRenderPlayerScores(currentTick, lastTick);
        } else if (curState != PBTBLMainScreenState::MAIN_BALLSAVED) {
            pbeRenderPlayerScores(currentTick, lastTick, false);
        }
    }

    // Render status text with fade effects
    pbeRenderStatusText(currentTick, lastTick);

    // Render status icons and values
    pbeRenderStatus(currentTick, lastTick);

    // NeoPixel animation: dramatic snake when ball save active, pulse-blue otherwise
    if (m_mainNeoPixelMode < 3) {
        bool wantSnake = m_playerStates[m_currentPlayer].ballSaveEnabled;
        if (wantSnake) {
            m_mainNeoPixelMode = 2;
            neoPixelSnake(0, 0, 20, 255, 100, 200, 5, true, IDO_NEOPIXEL0, 40);
        } else {
            m_mainNeoPixelMode = 1;
            neoPixelGradualFade(0, 0, 20, 0, 0, 200, IDO_NEOPIXEL0, 30, 800, false, true);
        }
    }

    return (true);
}

// Renders the normal main screen (score and messages)
// This is the default screen state - formerly MAIN_SHOWSCORE
bool PBEngine::pbeRenderMainScreenNormal(unsigned long currentTick, unsigned long lastTick){
    // Currently, all rendering is in the base function
    // This function exists for future expansion (e.g., special overlays)
    return (true);
}

// Renders the "Ball Saved" message when a ball save is triggered on drain
bool PBEngine::pbeRenderMainScreenBallSaved(unsigned long currentTick, unsigned long lastTick){
    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255); // Gold color
    gfxSetScaleFactor(m_StartMenuFontId, 1.0, false);
    gfxRenderString(m_StartMenuFontId, "Ball Saved", (ACTIVEDISPX + (1024 / 3)), ACTIVEDISPY + 350, 5, GFX_TEXTCENTER);
    return (true);
}

// Renders the "Inn Open!" message when all 3 inn lanes are completed
bool PBEngine::pbeRenderMainScreenInnOpen(unsigned long currentTick, unsigned long lastTick){
    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255); // Gold color
    gfxSetScaleFactor(m_StartMenuFontId, 1.0, false);
    gfxRenderString(m_StartMenuFontId, "Inn Open!", (ACTIVEDISPX + (1024 / 3)), ACTIVEDISPY + 350, 5, GFX_TEXTCENTER);
    return (true);
}

// Renders the "Key Obtained!" message when all 3 key targets are hit
bool PBEngine::pbeRenderMainScreenKeyObtained(unsigned long currentTick, unsigned long lastTick){
    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255); // Gold color
    gfxSetScaleFactor(m_StartMenuFontId, 1.0, false);
    gfxRenderString(m_StartMenuFontId, "Key Obtained!", (ACTIVEDISPX + (1024 / 3)), ACTIVEDISPY + 350, 5, GFX_TEXTCENTER);
    return (true);
}

// Renders the extra ball award screen with video
bool PBEngine::pbeRenderMainScreenExtraBall(unsigned long currentTick, unsigned long lastTick){
    static unsigned long lastScreenStartTick = 0;
    
    // Load and setup video player if not already done
    if (!m_extraBallVideoLoaded) {
        // Reset screen tracking when reloading video
        lastScreenStartTick = 0;
        
        if (!m_extraBallVideoPlayer) {
            m_extraBallVideoPlayer = new PBVideoPlayer(this, &m_soundSystem);
        }
        
        // Load the extra ball video (initial position, will be centered after loading)
        m_extraBallVideoSpriteId = m_extraBallVideoPlayer->pbvpLoadVideo(
            "src/user/resources/videos/extraball.mp4",
            0, 0,
            false  // Don't keep resident
        );
        
        if (m_extraBallVideoSpriteId != NOSPRITE) {
            // Set scale to 95%
            m_extraBallVideoPlayer->pbvpSetScaleFactor(0.95f);
            
            // Get video dimensions to calculate centered position
            stVideoInfo videoInfo = m_extraBallVideoPlayer->pbvpGetVideoInfo();
            int scaledWidth = (int)(videoInfo.width * 0.95f);
            int scaledHeight = (int)(videoInfo.height * 0.95f);
            
            // Center in active display area (1024x768), aligned with score text center
            // Score text is centered at ACTIVEDISPX + (1024/3) = ACTIVEDISPX + 341
            int centerX = ACTIVEDISPX + (1024 / 3);  // Same X as score text
            int centerY = ACTIVEDISPY + 350;  // Same Y as score text
            
            // Calculate upper-left position for centered video, offset 4 pixels left
            int videoX = centerX - (scaledWidth / 2) - 4;
            int videoY = centerY - (scaledHeight / 2) + 1;
            
            m_extraBallVideoPlayer->pbvpSetXY(videoX, videoY);
            m_extraBallVideoLoaded = true;
        } else {
            m_extraBallVideoLoaded = false;
            pbeSendConsole("ERROR: Failed to load extra ball video");
            return (false);
        }
    }
    
    // Update and render the video
    if (m_extraBallVideoLoaded && m_extraBallVideoPlayer) {
        bool shouldRender = true;
        
        // Check if this is a new screen activation
        if (lastScreenStartTick != m_currentScreenStartTick) {
            // Screen was just queued - restart video from beginning
            m_extraBallVideoPlayer->pbvpSeekTo(0.0f);
            m_extraBallVideoPlayer->pbvpSetVolume(0);  // Set volume to 0 for silent playback
            m_extraBallVideoPlayer->pbvpPlay();
            m_extraBallVideoPlayer->pbvpUpdate(currentTick);  // Update to load frame 0
            lastScreenStartTick = m_currentScreenStartTick;
            shouldRender = false;  // Skip rendering this frame to avoid showing stale frame
        } else {
            // Normal playback - check if we need to loop
            pbvPlaybackState videoState = m_extraBallVideoPlayer->pbvpGetPlaybackState();
            
            if (videoState == PBV_FINISHED) {
                // Video finished - restart from beginning
                m_extraBallVideoPlayer->pbvpSeekTo(0.0f);
                m_extraBallVideoPlayer->pbvpPlay();
                m_extraBallVideoPlayer->pbvpUpdate(currentTick);  // Load frame 0
                shouldRender = false;  // Skip rendering to avoid showing stale last frame
            } else {
                // Normal playback
                m_extraBallVideoPlayer->pbvpUpdate(currentTick);
            }
        }
        
        if (shouldRender) {
            m_extraBallVideoPlayer->pbvpRender();
        }
    }
    
    return (true);
}

// Main organizer for main screen rendering - now uses screen manager
bool PBEngine::pbeRenderMainScreen(unsigned long currentTick, unsigned long lastTick, PBTBLMainScreenState subScreenState){
    
    // Always render the base screen (background, scores, status)
    if (!pbeRenderMainScreenBase(currentTick, lastTick)) {
        return (false);
    }
    
    // Render specific screen content based on main substate
    switch (subScreenState) {
        case PBTBLMainScreenState::MAIN_EXTRABALL:
            pbeRenderMainScreenExtraBall(currentTick, lastTick);
            break;
        case PBTBLMainScreenState::MAIN_BALLSAVED:
            pbeRenderMainScreenBallSaved(currentTick, lastTick);
            break;
        case PBTBLMainScreenState::MAIN_INN_OPEN:
            pbeRenderMainScreenInnOpen(currentTick, lastTick);
            break;
        case PBTBLMainScreenState::MAIN_KEY_OBTAINED:
            pbeRenderMainScreenKeyObtained(currentTick, lastTick);
            break;
        case PBTBLMainScreenState::MAIN_NORMAL:
        default:
            pbeRenderMainScreenNormal(currentTick, lastTick);
            break;
    }
    
    // Render the main screen divider bars, they need to be on top to overlay the video and score text properly, so we render them at the end of this function
    gfxRenderSprite(m_PBTBLMainScreenBGId, ACTIVEDISPX, ACTIVEDISPY);

    return (true);
}

// ========================================================================
// PBTBL_MAIN: Supporting Render Functions
// ========================================================================

void PBEngine::pbeRenderPlayerScores(unsigned long currentTick, unsigned long lastTick, bool renderMainScore){
    if (renderMainScore) {
    // Calculate fade-in alpha for main score
    
    unsigned int mainAlpha = 255;

    if (m_mainScoreAnimActive) {
        float elapsedSec = (currentTick - m_mainScoreAnimStartTick) / 1000.0f;
        float animDuration = 1.5f;
        
        if (elapsedSec >= animDuration) {
            m_mainScoreAnimActive = false;
            mainAlpha = 255;
        } else {
            // Linear fade from 0 to 255
            float progress = elapsedSec / animDuration;
            mainAlpha = static_cast<unsigned int>(progress * 255.0f);
        }
    }
    
    // Render the current player's score in the center (smaller white text)
    gfxSetColor(m_StartMenuFontId, 255, 255, 255, mainAlpha);
    gfxSetScaleFactor(m_StartMenuFontId, 0.6, false);
    
    // Render "Player X" label above the score
    std::string playerLabel = "Player " + std::to_string(m_currentPlayer + 1);
    gfxRenderString(m_StartMenuFontId, playerLabel, (ACTIVEDISPX+(1024/3)), ACTIVEDISPY + 280, 5, GFX_TEXTCENTER);
    
    // Calculate animated score for main player
    unsigned long displayScore = m_playerStates[m_currentPlayer].score;
    
    if (m_playerStates[m_currentPlayer].score != m_playerStates[m_currentPlayer].inProgressScore) {
        // Check if score changed during animation - if so, update inProgressScore to current display value and restart
        if (m_playerStates[m_currentPlayer].score != m_playerStates[m_currentPlayer].previousScore) {
            // Calculate current display score before restarting
            if (m_playerStates[m_currentPlayer].scoreUpdateStartTick > 0) {
                unsigned long elapsedMS = currentTick - m_playerStates[m_currentPlayer].scoreUpdateStartTick;
                if (elapsedMS < UPDATESCOREMS) {
                    float progress = (float)elapsedMS / (float)UPDATESCOREMS;
                    long scoreDiff = m_playerStates[m_currentPlayer].previousScore - m_playerStates[m_currentPlayer].inProgressScore;
                    displayScore = m_playerStates[m_currentPlayer].inProgressScore + (unsigned long)(scoreDiff * progress);
                    displayScore = ((displayScore + 9) / 10) * 10;
                    // Set inProgressScore to current display value so score never goes down
                    m_playerStates[m_currentPlayer].inProgressScore = displayScore;
                }
            }
            // Restart animation from current position to new score
            m_playerStates[m_currentPlayer].scoreUpdateStartTick = currentTick;
            m_playerStates[m_currentPlayer].previousScore = m_playerStates[m_currentPlayer].score;
        }
        
        // Initialize animation start time if not set
        if (m_playerStates[m_currentPlayer].scoreUpdateStartTick == 0) {
            m_playerStates[m_currentPlayer].scoreUpdateStartTick = currentTick;
        }
        
        // Calculate time elapsed since animation started
        unsigned long elapsedMS = currentTick - m_playerStates[m_currentPlayer].scoreUpdateStartTick;
        
        if (elapsedMS >= UPDATESCOREMS) {
            // Animation complete, show final score
            displayScore = m_playerStates[m_currentPlayer].score;
            m_playerStates[m_currentPlayer].inProgressScore = m_playerStates[m_currentPlayer].score;
            m_playerStates[m_currentPlayer].previousScore = m_playerStates[m_currentPlayer].score;
            m_playerStates[m_currentPlayer].scoreUpdateStartTick = 0;
        } else {
            // Calculate interpolated score based on elapsed time
            float progress = (float)elapsedMS / (float)UPDATESCOREMS;
            long scoreDiff = m_playerStates[m_currentPlayer].score - m_playerStates[m_currentPlayer].inProgressScore;
            displayScore = m_playerStates[m_currentPlayer].inProgressScore + (unsigned long)(scoreDiff * progress);
            
            // Round up to nearest 10
            displayScore = ((displayScore + 9) / 10) * 10;
        }
    } else {
        // Scores are equal, reset animation tracking
        m_playerStates[m_currentPlayer].previousScore = m_playerStates[m_currentPlayer].score;
        m_playerStates[m_currentPlayer].scoreUpdateStartTick = 0;
    }
    
    // Render the current player's score with commas
    std::string scoreText = formatScoreWithCommas(displayScore);
    gfxSetScaleFactor(m_StartMenuFontId, 1.2, false);
    gfxRenderString(m_StartMenuFontId, scoreText, (ACTIVEDISPX+(1024/3)), ACTIVEDISPY + 350, 5, GFX_TEXTCENTER);
    } // if (renderMainScore)
    
    // Render other player scores at the bottom (small grey text)
    gfxSetColor(m_StartMenuFontId, 128, 128, 128, 255); // Light grey color for visibility
    gfxSetScaleFactor(m_StartMenuFontId, 0.375, false); // 25% smaller than 0.5
    
    // Render the other player scores at fixed positions based on thirds of active region
    // Active region is approximately 1100 pixels wide
    int activeWidth = 1024;
    int thirdWidth = activeWidth / 3;
    
    int leftX = ACTIVEDISPX + 10;                    // 10 pixels from start (far left)
    int middleX = ACTIVEDISPX + thirdWidth + 10;     // Start of 2nd third + 10 pixels
    int rightX = ACTIVEDISPX + (2 * thirdWidth) + 10; // Start of 3rd third + 10 pixels
    int positions[3] = {leftX, middleX, rightX};
    
    int slotIndex = 0;
    for (int i = 0; i < 4; i++) {
        // Skip current player and players not in this game
        if (i == m_currentPlayer || !m_playerStates[i].inGame) continue;
        
        // Only render up to 3 secondary scores
        if (slotIndex >= 3) break;
        
        // Calculate Y offset for scroll-up animation
        int yOffset = 0;
        if (m_secondaryScoreAnims[slotIndex].animationActive) {
            float elapsedSec = (currentTick - m_secondaryScoreAnims[slotIndex].animStartTick) / 1000.0f;
            
            if (elapsedSec >= m_secondaryScoreAnims[slotIndex].animDurationSec) {
                m_secondaryScoreAnims[slotIndex].animationActive = false;
                m_secondaryScoreAnims[slotIndex].currentYOffset = 0;
            } else {
                // Linear interpolation from starting offset to 0
                float progress = elapsedSec / m_secondaryScoreAnims[slotIndex].animDurationSec;
                m_secondaryScoreAnims[slotIndex].currentYOffset = (int)(50 * (1.0f - progress));
            }
            
            yOffset = m_secondaryScoreAnims[slotIndex].currentYOffset;
        }
        
        // Render "PX: score" format, left-justified at the position
        std::string playerScoreText = "P" + std::to_string(i + 1) + ": " + formatScoreWithCommas(m_playerStates[i].score);
        gfxRenderString(m_StartMenuFontId, playerScoreText, positions[slotIndex], ACTIVEDISPY + 725 + yOffset, 3, GFX_TEXTLEFT);
        
        slotIndex++;
    }
}

void PBEngine::pbeSetStatusText(int index, const std::string& text){
    if (index >= 0 && index <= 1) {
        m_statusText[index] = text;
    }
}

void PBEngine::pbeRenderStatusText(unsigned long currentTick, unsigned long lastTick){
    // Calculate position: right justified to (active width - 1/3 active width)
    // Active width is 1024, so 1/3 is ~341, right edge is at 1024 - 341 = 683
    int rightX = ACTIVEDISPX + 683;
    int yPos = ACTIVEDISPY + 660; // Same vertical position as Ball text
    
    // Render the current ball indicator in lower left (same color as main score)
    gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 0.35, false);
    std::string ballText = "Ball: " + std::to_string(m_playerStates[m_currentPlayer].currentBall);
    if (m_playerStates[m_currentPlayer].extraBallEnabled) ballText += "+1";
    gfxRenderString(m_StartMenuFontId, ballText, ACTIVEDISPX + 10, yPos, 3, GFX_TEXTLEFT);
    
    // Check if both strings are empty - skip rendering entirely
    if (m_statusText[0].empty() && m_statusText[1].empty()) {
        return;
    }
    
    // Initialize fade start time on first call
    if (m_statusTextFadeStart == 0) {
        m_statusTextFadeStart = currentTick;
    }
    
    // Check if we're switching text
    if (m_currentActiveText != m_previousActiveText) {
        // Only switch if the new text is not empty
        if (m_statusText[m_currentActiveText].empty()) {
            // New text is empty, revert to previous
            m_currentActiveText = m_previousActiveText;
        } else if (m_statustextFadeIn) {
            // Text changed - start fade out of old text if we were fading in
            m_statustextFadeIn = false;
            m_statusTextFadeStart = currentTick;
        }
    }
    
    // Calculate fade alpha
    unsigned int alpha = 0;
    float fadeDuration = 500.0f; // 500ms fade time
    float elapsedSec = (currentTick - m_statusTextFadeStart) / 1000.0f;
    float elapsedMs = elapsedSec * 1000.0f;
    
    // Determine which text to render based on fade state
    int textToRender = m_statustextFadeIn ? m_currentActiveText : m_previousActiveText;
    
    if (!m_statustextFadeIn) {
        // Fading out
        if (elapsedMs >= fadeDuration) {
            // Fade out complete, start fading in new text
            alpha = 0;
            m_statustextFadeIn = true;
            m_previousActiveText = m_currentActiveText;
            m_statusTextFadeStart = currentTick;
        } else {
            // Linear fade from 255 to 0
            float progress = elapsedMs / fadeDuration;
            alpha = static_cast<unsigned int>(255.0f * (1.0f - progress));
        }
    } else {
        // Fading in
        if (elapsedMs >= fadeDuration) {
            // Fade in complete
            alpha = 255;
            
            // Start display timer if not already started
            if (m_statusTextDisplayStart == 0) {
                m_statusTextDisplayStart = currentTick;
            }
            
            // Check if other text entry is not empty and display duration has elapsed
            int otherTextIndex = (m_currentActiveText == 0) ? 1 : 0;
            if (!m_statusText[otherTextIndex].empty()) {
                float displayDuration = 4000.0f; // 3 seconds display time
                float displayElapsedMs = (currentTick - m_statusTextDisplayStart);
                
                if (displayElapsedMs >= displayDuration) {
                    // Switch to other text (will trigger fade out on next frame)
                    m_currentActiveText = otherTextIndex;
                    m_statusTextDisplayStart = 0;
                }
            }
        } else {
            // Linear fade from 0 to 255
            float progress = elapsedMs / fadeDuration;
            alpha = static_cast<unsigned int>(255.0f * progress);
        }
    }
    
    // Render the current status text
    gfxSetColor(m_StartMenuFontId, 255, 255, 255, alpha);
    gfxSetScaleFactor(m_StartMenuFontId, 0.35, false);
    gfxRenderString(m_StartMenuFontId, m_statusText[textToRender], rightX, yPos, 3, GFX_TEXTRIGHT);
}

bool PBEngine::pbeRenderStatus(unsigned long currentTick, unsigned long lastTick){
    
    // Static position variables for character circles
    static const int circle1X = ACTIVEDISPX + 700;
    static const int circle1Y = ACTIVEDISPY + 20;
    static const int circle2X = ACTIVEDISPX + 860;
    static const int circle2Y = ACTIVEDISPY + 20;
    static const int circle3X = ACTIVEDISPX + 780;
    static const int circle3Y = ACTIVEDISPY + 170;
    
    // Static position variables for resource icons
    static const int treasureX = ACTIVEDISPX + 720;
    static const int treasureY = ACTIVEDISPY + 350;
    static const int swordX = ACTIVEDISPX + 685;
    static const int swordY = ACTIVEDISPY + 467;
    static const int shieldX = ACTIVEDISPX + 845;
    static const int shieldY = ACTIVEDISPY + 467;
    static const int dungeonX = ACTIVEDISPX + 720;
    static const int dungeonY = ACTIVEDISPY + 580;
    
    // Get current player's game state
    pbGameState& playerState = m_playerStates[m_currentPlayer];
    
    // Render character headshots in the center of each circle
    // Headshots are 256x256, scaled down by another 10% from 0.45 to 0.405 (~104px)
    
    // Ranger (Archer) - upper left circle (Shidea) - right 30px, down 15px
    gfxSetScaleFactor(m_PBTBLArcherHeadshot256Id, 0.405, false);
    if (playerState.rangerJoined) {
        // Full color
        gfxSetColor(m_PBTBLArcherHeadshot256Id, 255, 255, 255, 255);
    } else {
        // 1/3 grayscale (85, 85, 85) at 50% transparency
        gfxSetColor(m_PBTBLArcherHeadshot256Id, 85, 85, 85, 128);
    }
    gfxRenderSprite(m_PBTBLArcherHeadshot256Id, circle1X + 37, circle1Y + 22);
    
    // Priest (Wolf) - upper right circle (Kahriel) - right 20px, down 10px
    gfxSetScaleFactor(m_PBTBLWolfHeadshot256Id, 0.405, false);
    if (playerState.priestJoined) {
        // Full color
        gfxSetColor(m_PBTBLWolfHeadshot256Id, 255, 255, 255, 255);
    } else {
        // 1/3 grayscale (85, 85, 85) at 50% transparency
        gfxSetColor(m_PBTBLWolfHeadshot256Id, 85, 85, 85, 128);
    }
    gfxRenderSprite(m_PBTBLWolfHeadshot256Id, circle2X + 27, circle2Y + 27);
    
    // Knight - lower middle circle (Caiphos) - right 25px, down 20px
    gfxSetScaleFactor(m_PBTBLKnightHeadshot256Id, 0.38, false);
    if (playerState.knightJoined) {
        // Full color
        gfxSetColor(m_PBTBLKnightHeadshot256Id, 255, 255, 255, 255);
    } else {
        // 1/3 grayscale (85, 85, 85) at 50% transparency
        gfxSetColor(m_PBTBLKnightHeadshot256Id, 85, 85, 85, 128);
    }
    gfxRenderSprite(m_PBTBLKnightHeadshot256Id, circle3X + 25, circle3Y + 31);
    
    // Render character circles
    gfxRenderSprite(m_PBTBLCharacterCircle256Id, circle1X, circle1Y);
    gfxRenderSprite(m_PBTBLCharacterCircle256Id, circle2X, circle2Y);
    gfxRenderSprite(m_PBTBLCharacterCircle256Id, circle3X, circle3Y);
    
    // Render level text at bottom of each circle
    gfxSetScaleFactor(m_StartMenuFontId, 0.3, false);
    
    // Ranger level - below and to the left of circle1
    std::string rangerLevelText = "Level: " + std::to_string(playerState.rangerLevel);
    if (playerState.rangerJoined) {
        // White color with black shadow
        gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
        gfxRenderShadowString(m_StartMenuFontId, rangerLevelText, circle1X + 14, circle1Y + 145, 3, GFX_TEXTLEFT, 0, 0, 0, 255, 1);
    }
    // Priest level - below and to the right of circle2
    std::string priestLevelText = "Level: " + std::to_string(playerState.priestLevel);
    if (playerState.priestJoined) {
        // White color with black shadow
        gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
        gfxRenderShadowString(m_StartMenuFontId, priestLevelText, circle2X + 128 + 10, circle2Y + 145, 3, GFX_TEXTRIGHT, 0, 0, 0, 255, 1);
    }
    // Knight level - bottom of circle3
    std::string knightLevelText = "Level: " + std::to_string(playerState.knightLevel);
    if (playerState.knightJoined) {
        // White color with black shadow
        gfxSetColor(m_StartMenuFontId, 255, 255, 255, 255);
        gfxRenderShadowString(m_StartMenuFontId, knightLevelText, circle3X + 78, circle3Y - 15, 3, GFX_TEXTCENTER, 0, 0, 0, 255, 1);
    }
    // Render resource icons
    gfxRenderSprite(m_PBTBLTreasure256Id, treasureX, treasureY);
    gfxRenderSprite(m_PBTBLSword256Id, swordX, swordY);
    gfxRenderSprite(m_PBTBLShield256Id, shieldX, shieldY);
    gfxRenderSprite(m_PBTBLDungeon256Id, dungeonX, dungeonY);

    // Eye-blink: visible for 1 second, then hidden for a random 1-15 second interval.
    static const int blinkX = dungeonX + (int)(256 * 0.42f / 2) - 12;  // horizontal center
    static const int blinkY = dungeonY + 30;                              // top of dungeon sprite + 30
    if (currentTick >= m_dungeonBlinkNextTick) {
        if (currentTick < m_dungeonBlinkNextTick + 1000) {
            gfxRenderSprite(m_PBTBLDungeonEyesBlinkId, blinkX, blinkY);
        } else {
            // Blink finished — schedule next one with a random 1-15 second delay
            m_dungeonBlinkNextTick = currentTick + (rand() % 14001) + 1000;
        }
    }

    // Render character names with gold color and shadow
    gfxSetColor(m_StartMenuFontId, 235, 176, 20, 255);
    gfxSetScaleFactor(m_StartMenuFontId, 0.3, false);
    
    // "Shidea" above top left circle (center of circle is at circle1X + 64)
    gfxRenderShadowString(m_StartMenuFontId, "Shidea", circle1X + 77, circle1Y - 15, 4, GFX_TEXTCENTER, 150, 100, 0, 255, 2);
    
    // "Kahriel" above top right circle (center of circle is at circle2X + 64)
    gfxRenderShadowString(m_StartMenuFontId, "Kahriel", circle2X + 78, circle2Y - 15, 4, GFX_TEXTCENTER, 150, 100, 0, 255, 2);
    
    // "Caiphos" below middle circle (bottom is at circle3Y + 128)
    gfxRenderShadowString(m_StartMenuFontId, "Caiphos", circle3X + 76, circle3Y + 142, 4, GFX_TEXTCENTER, 150, 100, 0, 255, 2);
    
    // Render resource values to the right of icons
    // Icon text scaled to 0.7 (2x bigger than 0.35)
    gfxSetScaleFactor(m_StartMenuFontId, 0.7, false);
    
    // Gold value (bright gold color)
    gfxSetColor(m_StartMenuFontId, 255, 215, 0, 255);
    std::string goldText = std::to_string(playerState.goldValue);
    gfxRenderShadowString(m_StartMenuFontId, goldText, treasureX + 115, treasureY + 30, 3, GFX_TEXTLEFT, 180, 140, 0, 255, 2);
    
    // Attack value (fire red/orange color)
    gfxSetColor(m_StartMenuFontId, 255, 80, 20, 255);
    std::string attackText = std::to_string(playerState.attackValue);
    gfxRenderShadowString(m_StartMenuFontId, attackText, swordX + 105, swordY + 30, 3, GFX_TEXTLEFT, 180, 40, 10, 255, 2);
    
    // Defense value (steel blue-grey color)
    gfxSetColor(m_StartMenuFontId, 43, 66, 69, 255);
    std::string defenseText = std::to_string(playerState.defenseValue);
    gfxRenderShadowString(m_StartMenuFontId, defenseText, shieldX + 105, shieldY + 30, 3, GFX_TEXTLEFT, 20, 30, 32, 255, 2);
    
    // Dungeon floor and level (darker stone grey color)
    // Dungeon text scaled to 0.525 (1.5x bigger than 0.35)
    gfxSetScaleFactor(m_StartMenuFontId, 0.525, false);
    gfxSetColor(m_StartMenuFontId, 81, 79, 122, 255);
    std::string floorText = "Floor: " + std::to_string(playerState.dungeonFloor);
    std::string levelText = "Level: " + std::to_string(playerState.dungeonLevel);
    gfxRenderShadowString(m_StartMenuFontId, floorText, dungeonX + 123, dungeonY + 15, 3, GFX_TEXTLEFT, 27, 24, 48, 255, 2);
    gfxRenderShadowString(m_StartMenuFontId, levelText, dungeonX + 123, dungeonY + 60, 3, GFX_TEXTLEFT, 27, 24, 48, 255, 2);
    
    return (true);
}

// ========================================================================
// PBTBL_MAIN: State Update Function
// ========================================================================

void PBEngine::pbeUpdateStateMain(stInputMessage inputMessage){

    // START button: try to add a player mid-game
    if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
        if (inputMessage.inputId == IDI_START) {
            if (pbeTryAddPlayer()) {
                m_soundSystem.pbsPlayEffect(SOUNDSWORDCUT);
            }
        }
    }

    // Slingshot hits: +1000 score, sword cut sound, check extra ball milestone
    if (inputMessage.inputMsg == PB_IMSG_SLING && inputMessage.inputState == PB_ON) {
        if (inputMessage.inputId == IDI_LSLING || inputMessage.inputId == IDI_RSLING) {
            addPlayerScore(1000);
            m_soundSystem.pbsPlayEffect(SOUNDSWORDCUT);

            // Check extra ball milestone (every 10,000 points)
            pbGameState& ps = m_playerStates[m_currentPlayer];
            unsigned long threshold = (ps.score / 10000UL) * 10000UL;
            if (threshold > 0 && threshold > ps.lastExtraBallThreshold) {
                ps.lastExtraBallThreshold = threshold;
                pbeRequestScreen(PBTableState::PBTBL_MAIN,
                                 static_cast<int>(PBTBLMainScreenState::MAIN_EXTRABALL),
                                 ScreenPriority::PRIORITY_HIGH, 3900, false);
                if (!ps.extraBallEnabled) {
                    ps.extraBallEnabled = true;
                    ps.ballSaveEnabled  = true;
                    SendOutputMsg(PB_OMSG_LED, IDO_SAVELED, PB_ON, false);
                }
            }
        }
    }

    // Pop bumper hits: +50 score, +1 gold
    if (inputMessage.inputMsg == PB_IMSG_POPBUMPER && inputMessage.inputState == PB_ON) {
        if (inputMessage.inputId == IDI_POP1 ||
            inputMessage.inputId == IDI_POP2 ||
            inputMessage.inputId == IDI_POP3) {
            addPlayerScore(50);
            pbGameState& ps = m_playerStates[m_currentPlayer];
            ps.goldValue++;
        }
    }

    // Inn lane sensors: +250 score, light the matching LED.
    // Left/right flipper shifts all inn lane LEDs; if all three light → "Inn Open!".
    if (inputMessage.inputMsg == PB_IMSG_SENSOR && inputMessage.inputState == PB_ON) {
        bool innLaneHit = false;
        if (inputMessage.inputId == IDI_INN1 && !m_innLaneLEDOn[0]) {
            m_innLaneLEDOn[0] = true;
            SendOutputMsg(PB_OMSG_LED, IDO_INN1LED, PB_ON, false);
            innLaneHit = true;
        }
        else if (inputMessage.inputId == IDI_INN2 && !m_innLaneLEDOn[1]) {
            m_innLaneLEDOn[1] = true;
            SendOutputMsg(PB_OMSG_LED, IDO_INN2LED, PB_ON, false);
            innLaneHit = true;
        }
        else if (inputMessage.inputId == IDI_INN3 && !m_innLaneLEDOn[2]) {
            m_innLaneLEDOn[2] = true;
            SendOutputMsg(PB_OMSG_LED, IDO_INN3LED, PB_ON, false);
            innLaneHit = true;
        }

        if (innLaneHit) {
            addPlayerScore(250);
            // Check if all three inn lanes are lit
            if (m_innLaneLEDOn[0] && m_innLaneLEDOn[1] && m_innLaneLEDOn[2]) {
                pbGameState& ps = m_playerStates[m_currentPlayer];
                ps.bInnOpen = true;
                // Reset inn lane LEDs
                m_innLaneLEDOn[0] = m_innLaneLEDOn[1] = m_innLaneLEDOn[2] = false;
                SendOutputMsg(PB_OMSG_LED, IDO_INN1LED, PB_OFF, false);
                SendOutputMsg(PB_OMSG_LED, IDO_INN2LED, PB_OFF, false);
                SendOutputMsg(PB_OMSG_LED, IDO_INN3LED, PB_OFF, false);
                // Flash "Inn Open!" for 2 seconds
                pbeRequestScreen(PBTableState::PBTBL_MAIN,
                                 static_cast<int>(PBTBLMainScreenState::MAIN_INN_OPEN),
                                 ScreenPriority::PRIORITY_HIGH, 2000, false);
            }
        }
    }

    // Flipper buttons: shift inn lane LEDs left or right (circular rotate)
    if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
        if (inputMessage.inputId == IDI_LFLIP) {
            // Rotate LEDs left: [1]→[0], [2]→[1], [0]→[2]
            bool temp = m_innLaneLEDOn[0];
            m_innLaneLEDOn[0] = m_innLaneLEDOn[1];
            m_innLaneLEDOn[1] = m_innLaneLEDOn[2];
            m_innLaneLEDOn[2] = temp;
            SendOutputMsg(PB_OMSG_LED, IDO_INN1LED, m_innLaneLEDOn[0] ? PB_ON : PB_OFF, false);
            SendOutputMsg(PB_OMSG_LED, IDO_INN2LED, m_innLaneLEDOn[1] ? PB_ON : PB_OFF, false);
            SendOutputMsg(PB_OMSG_LED, IDO_INN3LED, m_innLaneLEDOn[2] ? PB_ON : PB_OFF, false);
        }
        else if (inputMessage.inputId == IDI_RFLIP) {
            // Rotate LEDs right: [1]→[2], [0]→[1], [2]→[0]
            bool temp = m_innLaneLEDOn[2];
            m_innLaneLEDOn[2] = m_innLaneLEDOn[1];
            m_innLaneLEDOn[1] = m_innLaneLEDOn[0];
            m_innLaneLEDOn[0] = temp;
            SendOutputMsg(PB_OMSG_LED, IDO_INN1LED, m_innLaneLEDOn[0] ? PB_ON : PB_OFF, false);
            SendOutputMsg(PB_OMSG_LED, IDO_INN2LED, m_innLaneLEDOn[1] ? PB_ON : PB_OFF, false);
            SendOutputMsg(PB_OMSG_LED, IDO_INN3LED, m_innLaneLEDOn[2] ? PB_ON : PB_OFF, false);
        }
    }

    // Key target sensors: +250 score, light the matching LED.
    // When all three are lit → "Key Obtained!". Flippers do NOT shift these LEDs.
    if (inputMessage.inputMsg == PB_IMSG_TARGET && inputMessage.inputState == PB_ON) {
        bool keyTargetHit = false;
        if (inputMessage.inputId == IDI_KEY1 && !m_keyTargetLEDOn[0]) {
            m_keyTargetLEDOn[0] = true;
            SendOutputMsg(PB_OMSG_LED, IDO_KEY1LED, PB_ON, false);
            keyTargetHit = true;
        }
        else if (inputMessage.inputId == IDI_KEY2 && !m_keyTargetLEDOn[1]) {
            m_keyTargetLEDOn[1] = true;
            SendOutputMsg(PB_OMSG_LED, IDO_KEY2LED, PB_ON, false);
            keyTargetHit = true;
        }
        else if (inputMessage.inputId == IDI_KEY3 && !m_keyTargetLEDOn[2]) {
            m_keyTargetLEDOn[2] = true;
            SendOutputMsg(PB_OMSG_LED, IDO_KEY3LED, PB_ON, false);
            keyTargetHit = true;
        }

        if (keyTargetHit) {
            addPlayerScore(250);
            // Check if all three key targets are lit
            if (m_keyTargetLEDOn[0] && m_keyTargetLEDOn[1] && m_keyTargetLEDOn[2]) {
                pbGameState& ps = m_playerStates[m_currentPlayer];
                ps.bKeyObtained = true;
                // Flash "Key Obtained!" for 2 seconds
                pbeRequestScreen(PBTableState::PBTBL_MAIN,
                                 static_cast<int>(PBTBLMainScreenState::MAIN_KEY_OBTAINED),
                                 ScreenPriority::PRIORITY_HIGH, 2000, false);
            }
        }
    }

    // Inlane sensors: LED on when sensor hit, stays on. When both lit → activate ball save.
    if (inputMessage.inputMsg == PB_IMSG_SENSOR && inputMessage.inputState == PB_ON) {
        if (inputMessage.inputId == IDI_LINLANE && !m_leftInlaneLEDOn) {
            m_leftInlaneLEDOn = true;
            SendOutputMsg(PB_OMSG_LED, IDO_LINLANELED, PB_ON, false);
        }
        else if (inputMessage.inputId == IDI_RINLANE && !m_rightInlaneLEDOn) {
            m_rightInlaneLEDOn = true;
            SendOutputMsg(PB_OMSG_LED, IDO_RINLANELED, PB_ON, false);
        }

        // Both inlanes lit → activate ball save, start 5-second timer
        // Leave inlane LEDs on while save is active; they turn off when the timer expires
        if (m_leftInlaneLEDOn && m_rightInlaneLEDOn) {
            pbGameState& ps = m_playerStates[m_currentPlayer];
            ps.ballSaveEnabled = true;
            SendOutputMsg(PB_OMSG_LED, IDO_SAVELED, PB_ON, false);
            pbeSetTimer(BALLSAVE_TIMER_ID, 5000);
        }
    }

    // Ball save timer expiry: turn off SAVE LED, both inlane LEDs, and clear flags
    if (inputMessage.inputMsg == PB_IMSG_TIMER &&
        inputMessage.inputId == BALLSAVE_TIMER_ID) {
        pbGameState& ps = m_playerStates[m_currentPlayer];
        ps.ballSaveEnabled = false;
        m_leftInlaneLEDOn  = false;
        m_rightInlaneLEDOn = false;
        SendOutputMsg(PB_OMSG_LED, IDO_SAVELED, PB_OFF, false);
        SendOutputMsg(PB_OMSG_LED, IDO_LINLANELED, PB_OFF, false);
        SendOutputMsg(PB_OMSG_LED, IDO_RINLANELED, PB_OFF, false);
    }

    // Ball drain sensor
    if (inputMessage.inputMsg == PB_IMSG_SENSOR && inputMessage.inputState == PB_ON &&
        inputMessage.inputId == IDI_BALLDRAIN) {

        pbGameState& ps = m_playerStates[m_currentPlayer];
        if (ps.ballSaveEnabled) {
            // Ball save active – return ball, clear save and extra ball flags
            ps.ballSaveEnabled  = false;
            ps.extraBallEnabled = false;
            pbeActivatePlayer(m_currentPlayer);   // resets LEDs, save LED off, NeoPixel restart
            if (m_hopperDevice) {
                m_hopperDevice->pbdEnable(true);
                m_hopperDevice->pdbStartRun();
            }
            // Show "Ball Saved" message for 2 seconds
            pbeRequestScreen(PBTableState::PBTBL_MAIN,
                             static_cast<int>(PBTBLMainScreenState::MAIN_BALLSAVED),
                             ScreenPriority::PRIORITY_HIGH, 2000, false);
        } else {
            // No save – advance ball, rotate players or end game
            ps.currentBall++;
            if (ps.currentBall > m_saveFileData.ballsPerGame) {
                ps.enabled = false;
            }

            // Cancel any active ball-save timer so it doesn't fire in the next state
            pbeTimerStop(BALLSAVE_TIMER_ID);

            // Find next enabled player (circular search)
            int nextPlayer = -1;
            for (int i = 1; i <= 4; i++) {
                int candidate = (static_cast<int>(m_currentPlayer) + i) % 4;
                if (m_playerStates[candidate].enabled) {
                    nextPlayer = candidate;
                    break;
                }
            }

            if (nextPlayer < 0) {
                // No more enabled players – go to game end
                m_tableState = PBTableState::PBTBL_GAMEEND;
                m_tableSubScreenState = static_cast<int>(PBTBLGameEndState::GAMEEND_INIT);
                m_gameEndInitialized = false;
                pbeClearScreenRequests();
                pbeRequestScreen(PBTableState::PBTBL_GAMEEND,
                                 static_cast<int>(PBTBLGameEndState::GAMEEND_INIT),
                                 ScreenPriority::PRIORITY_LOW, 0, true);
                return;
            }

            // Transition to PlayerEnd to show "Player X" and eject the next ball
            m_playerEndNextPlayer  = nextPlayer;
            m_playerEndInitialized = false;
            m_tableState           = PBTableState::PBTBL_PLAYEREND;
            m_tableSubScreenState  = static_cast<int>(PBTBLPlayerEndState::PLAYEREND_DISPLAY);
            pbeClearScreenRequests();
            pbeRequestScreen(PBTableState::PBTBL_PLAYEREND,
                             static_cast<int>(PBTBLPlayerEndState::PLAYEREND_DISPLAY),
                             ScreenPriority::PRIORITY_LOW, 0, true);
            return;
        }
    }

    // Update mode system (screen manager, mode state machine)
    pbeUpdateModeSystem(inputMessage, GetTickCountGfx());
}

// ========================================================================
// MODE SYSTEM IMPLEMENTATION
// ========================================================================

// Helper function to update mode system - reduces code duplication
void PBEngine::pbeUpdateModeSystem(stInputMessage inputMessage, unsigned long currentTick) {
    // Update mode system state
    pbeUpdateModeState(currentTick);
    
    // Update screen manager
    pbeUpdateScreenManager(currentTick);
    
    // Route input to current mode's update function
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    switch (modeState.currentMode) {
        case PBTableMode::MODE_NORMAL_PLAY:
            pbeUpdateNormalPlayMode(inputMessage, currentTick);
            break;
        case PBTableMode::MODE_MULTIBALL:
            pbeUpdateMultiballMode(inputMessage, currentTick);
            break;
        default:
            break;
    }
}

// Main mode state update function - called each frame to update current mode
void PBEngine::pbeUpdateModeState(unsigned long currentTick) {
    // Get current player's mode state
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    // Check if we should transition to a different mode
    if (pbeCheckModeTransition(currentTick)) {
        return; // Mode transition occurred, state will be updated next frame
    }
    
    // Update the current mode's state machine
    switch (modeState.currentMode) {
        case PBTableMode::MODE_NORMAL_PLAY: {
            // Update normal play mode sub-states
            switch (modeState.normalPlayState) {
                case PBNormalPlayState::NORMAL_IDLE:
                    // Check if ball became active
                    // In a real implementation, you'd check ball switches/sensors
                    // For now, we'll just remain in idle
                    break;
                    
                case PBNormalPlayState::NORMAL_ACTIVE:
                    // Ball is in play - normal gameplay logic here
                    // Check for mode qualification conditions
                    if (pbeCheckMultiballQualified()) {
                        modeState.multiballQualified = true;
                    }
                    break;
                    
                case PBNormalPlayState::NORMAL_DRAIN:
                    // Ball drained - handle end of ball
                    // Transition back to idle after a delay
                    if (currentTick - modeState.normalPlayStateStartTick > 2000) {
                        modeState.normalPlayState = PBNormalPlayState::NORMAL_IDLE;
                        modeState.normalPlayStateStartTick = currentTick;
                    }
                    break;
                    
                default:
                    break;
            }
            break;
        }
        
        case PBTableMode::MODE_MULTIBALL: {
            // Update multiball mode sub-states
            switch (modeState.multiballState) {
                case PBMultiballState::MULTIBALL_START:
                    // Starting multiball sequence
                    // Transition to active after start sequence
                    if (currentTick - modeState.multiballStateStartTick > 3000) {
                        modeState.multiballState = PBMultiballState::MULTIBALL_ACTIVE;
                        modeState.multiballStateStartTick = currentTick;
                        modeState.multiballCount = 2; // Start with 2 balls
                        
                        // Request high-priority screen
                        pbeRequestScreen(PBTableState::PBTBL_MAIN, 100, ScreenPriority::PRIORITY_HIGH, 5000, false);
                    }
                    break;
                    
                case PBMultiballState::MULTIBALL_ACTIVE:
                    // Multiball gameplay active
                    // In a real implementation, track balls in play
                    // For now, we'll end multiball after 20 seconds
                    if (currentTick - modeState.multiballStateStartTick > 20000) {
                        modeState.multiballState = PBMultiballState::MULTIBALL_ENDING;
                        modeState.multiballStateStartTick = currentTick;
                    }
                    break;
                    
                case PBMultiballState::MULTIBALL_ENDING:
                    // Transitioning back to normal play
                    if (currentTick - modeState.multiballStateStartTick > 2000) {
                        // Exit multiball mode and return to normal play
                        pbeExitMode(PBTableMode::MODE_MULTIBALL, currentTick);
                        pbeEnterMode(PBTableMode::MODE_NORMAL_PLAY, currentTick);
                    }
                    break;
                    
                default:
                    break;
            }
            break;
        }
        
        default:
            break;
    }
}

// Check if conditions are met to transition to a different mode
bool PBEngine::pbeCheckModeTransition(unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    // Check for mode transitions based on current mode
    switch (modeState.currentMode) {
        case PBTableMode::MODE_NORMAL_PLAY: {
            // Check if multiball should start
            if (modeState.multiballQualified && 
                modeState.normalPlayState == PBNormalPlayState::NORMAL_ACTIVE) {
                
                // For this example, we'll trigger multiball when qualified
                // In a real game, you might need a specific switch hit
                pbeExitMode(PBTableMode::MODE_NORMAL_PLAY, currentTick);
                pbeEnterMode(PBTableMode::MODE_MULTIBALL, currentTick);
                return true;
            }
            break;
        }
        
        case PBTableMode::MODE_MULTIBALL: {
            // Multiball handles its own exit in pbeUpdateModeState
            break;
        }
        
        default:
            break;
    }
    
    return false;
}

// Enter a new mode
void PBEngine::pbeEnterMode(PBTableMode newMode, unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    // Save previous mode
    modeState.previousMode = modeState.currentMode;
    modeState.currentMode = newMode;
    modeState.modeTransitionActive = true;
    modeState.modeTransitionStartTick = currentTick;
    
    // Initialize mode-specific state
    switch (newMode) {
        case PBTableMode::MODE_NORMAL_PLAY:
            modeState.normalPlayState = PBNormalPlayState::NORMAL_ACTIVE;
            modeState.normalPlayStateStartTick = currentTick;
            
            // Request normal play screen
            pbeRequestScreen(PBTableState::PBTBL_MAIN, static_cast<int>(PBTBLMainScreenState::MAIN_NORMAL),
                             ScreenPriority::PRIORITY_LOW, 0, true);
            break;
            
        case PBTableMode::MODE_MULTIBALL:
            modeState.multiballState = PBMultiballState::MULTIBALL_START;
            modeState.multiballStateStartTick = currentTick;
            modeState.multiballCount = 1;
            
            // Request normal main screen for multiball (no special multiball screen yet)
            // TODO: Create MAIN_MULTIBALL screen state if multiball needs its own display
            pbeRequestScreen(PBTableState::PBTBL_MAIN, static_cast<int>(PBTBLMainScreenState::MAIN_NORMAL),
                             ScreenPriority::PRIORITY_LOW, 0, true);
            break;
            
        default:
            break;
    }
}

// Exit a mode
void PBEngine::pbeExitMode(PBTableMode exitingMode, unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    // Clean up mode-specific state
    switch (exitingMode) {
        case PBTableMode::MODE_NORMAL_PLAY:
            // No special cleanup needed
            break;
            
        case PBTableMode::MODE_MULTIBALL:
            // Reset multiball state
            modeState.multiballQualified = false;
            modeState.multiballCount = 1;
            break;
            
        default:
            break;
    }
}

// Update normal play mode with input
void PBEngine::pbeUpdateNormalPlayMode(stInputMessage inputMessage, unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    // Handle inputs specific to normal play mode
    if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
        // Example: Left activate button changes state in normal play
        if (inputMessage.inputId == IDI_LACTIVATE) {
            if (modeState.normalPlayState == PBNormalPlayState::NORMAL_IDLE) {
                modeState.normalPlayState = PBNormalPlayState::NORMAL_ACTIVE;
                modeState.normalPlayStateStartTick = currentTick;
            }
        }
        // Right activate button drains ball
        else if (inputMessage.inputId == IDI_RACTIVATE) {
            if (modeState.normalPlayState == PBNormalPlayState::NORMAL_ACTIVE) {
                modeState.normalPlayState = PBNormalPlayState::NORMAL_DRAIN;
                modeState.normalPlayStateStartTick = currentTick;
            }
        }
    }
}

// Update multiball mode with input
void PBEngine::pbeUpdateMultiballMode(stInputMessage inputMessage, unsigned long currentTick) {
    ModeState& modeState = m_playerStates[m_currentPlayer].modeState;
    
    // Handle inputs specific to multiball mode
    if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {
        // Example: Add scoring in multiball
        if (inputMessage.inputId == IDI_LACTIVATE) {
            addPlayerScore(5000); // Higher scoring in multiball
        }
    }
}

// Check if multiball is qualified
bool PBEngine::pbeCheckMultiballQualified() {
    // Example qualification: Score over 50,000 points
    // In a real game, this would check specific targets, combos, etc.
    return (m_playerStates[m_currentPlayer].score > 50000);
}
