// Pinball_Table_ModeInTower.cpp:  PBTBL_INTOWER mode implementation
//   Handles the tower lock screen. Entered when IDI_TOWER sensor is triggered.
//   Reuses the main screen base rendering (background, scores, status) and
//   renders tower-specific content only within the main status/scoring area.
//   No exit condition yet; will be added in a future task.

// Copyright (c) 2025 Jeffrey D. Bock, unless otherwise noted. Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
// The license can be found here: <https://creativecommons.org/licenses/by-nc/4.0/>.
// Additional details can also be found in the license file in the root of the project.

#include "Pinball_Engine.h"
#include "Pinball_Table.h"
#include "PBDevice.h"

// ========================================================================
// PBTBL_INTOWER: Load Function
// ========================================================================

bool PBEngine::pbeLoadInTower() {

    if (m_inTowerLoaded) return (true);

    // InTower reuses the main screen background resources; ensure they are loaded
    if (!pbeLoadMainScreen()) return (false);

    m_TowerClimbId = gfxLoadSprite("TowerClimb", "src/user/resources/textures/towerclimb.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, true, true);
    gfxSetColor(m_TowerClimbId, 80, 80, 160, 255);

    // Load dungeon door grid sprites
    m_DoorOpenId = gfxLoadSprite("DoorOpen", "src/user/resources/textures/dooropen.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, true, true);
    gfxSetColor(m_DoorOpenId, 255, 255, 255, 255);

    m_DoorClosedId = gfxLoadSprite("DoorClosed", "src/user/resources/textures/doorclosed.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, true, true);
    gfxSetColor(m_DoorClosedId, 255, 255, 255, 255);

    m_DoorBlockedId = gfxLoadSprite("DoorBlocked", "src/user/resources/textures/doorblocked.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, true, true);
    gfxSetColor(m_DoorBlockedId, 255, 255, 255, 255);

    m_DoorWall1Id = gfxLoadSprite("DoorWall1", "src/user/resources/textures/doorwall1.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, true, true);
    gfxSetColor(m_DoorWall1Id, 255, 255, 255, 255);

    m_DoorWall2Id = gfxLoadSprite("DoorWall2", "src/user/resources/textures/doorwall2.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, true, true);
    gfxSetColor(m_DoorWall2Id, 255, 255, 255, 255);

    m_DoorLadderId = gfxLoadSprite("DoorLadder", "src/user/resources/textures/doorladder.png", GFX_PNG, GFX_NOMAP, GFX_CENTER, true, true);
    gfxSetColor(m_DoorLadderId, 255, 255, 255, 255);

    if (m_DoorOpenId == NOSPRITE || m_DoorClosedId == NOSPRITE || m_DoorBlockedId == NOSPRITE ||
        m_DoorWall1Id == NOSPRITE || m_DoorWall2Id == NOSPRITE || m_DoorLadderId == NOSPRITE) {
        pbeSendConsole("ERROR: Failed to load InTower door sprites");
        return (false);
    }

    m_inTowerLoaded = true;
    return (true);
}

// ========================================================================
// PBTBL_INTOWER: Dungeon Grid Initialization
// ========================================================================

// File-scope test variable: tracks which dungeon level is displayed in InTower
// (for testing only; not tied to player-state dungeonLevel)
static int s_testDungeonLevel = 1;

// Initialize the dungeon grid for a given player at the specified level.
//   playerNum : 0-3 (clamped)
//   level     : 1=linear 3-floor (col 1 only)
//               2=4-floor 2-col (cols 1-2, random ladders)
//               3=5-floor 3-col (cols 0-2, random ladders)
void PBEngine::pbeInitDungeonGrid(int playerNum, int level) {

    // Clamp inputs to valid ranges
    if (playerNum < 0) playerNum = 0;
    if (playerNum > 3) playerNum = 3;
    if (level < 1) level = 1;
    if (level > 3) level = 3;

    TowerDungeonGrid& grid = m_playerStates[playerNum].dungeonGrid;

    // Clear all cells to DOOR_NONE
    grid = TowerDungeonGrid();

    if (level == 1) {
        // ----------------------------------------------------------------
        // Level 1: linear 3-floor path, centre column only, no randomness
        // ----------------------------------------------------------------
        const int col = 1;
        const int numRows = 3;
        for (int r = 0; r < numRows; r++) {
            grid.cells[r][col].state = DoorState::DOOR_CLOSED;
        }
        // Ladders on floors 0 and 1; dragon lair on floor 2
        grid.cells[0][col].hasLadder   = true;
        grid.cells[1][col].hasLadder   = true;
        grid.cells[2][col].isDragonLair = true;

        // TODO: Initialize room metadata here (monsters, room type, etc.)

    } else if (level == 2) {
        // ----------------------------------------------------------------
        // Level 2: 4-floor path, cols 1-2, random ladder per row
        // ----------------------------------------------------------------
        const int activeCols[2] = { 1, 2 };
        const int numActiveCols  = 2;
        const int numRows        = 4;

        for (int r = 0; r < numRows; r++) {
            for (int c = 0; c < numActiveCols; c++) {
                grid.cells[r][activeCols[c]].state = DoorState::DOOR_CLOSED;
            }
        }
        // One random ladder per non-top row
        for (int r = 0; r < numRows - 1; r++) {
            int pick = rand() % numActiveCols;
            grid.cells[r][activeCols[pick]].hasLadder = true;
        }
        // One random dragon lair on top row
        {
            int pick = rand() % numActiveCols;
            grid.cells[numRows - 1][activeCols[pick]].isDragonLair = true;
        }

        // TODO: Initialize room metadata here (monsters, room type, etc.)

    } else {
        // ----------------------------------------------------------------
        // Level 3: 5-floor path, all 3 cols (0-2), random ladder per row
        // ----------------------------------------------------------------
        const int activeCols[3] = { 0, 1, 2 };
        const int numActiveCols  = 3;
        const int numRows        = 5;

        for (int r = 0; r < numRows; r++) {
            for (int c = 0; c < numActiveCols; c++) {
                grid.cells[r][activeCols[c]].state = DoorState::DOOR_CLOSED;
            }
        }
        // One random ladder per non-top row
        for (int r = 0; r < numRows - 1; r++) {
            int pick = rand() % numActiveCols;
            grid.cells[r][activeCols[pick]].hasLadder = true;
        }
        // One random dragon lair on top row
        {
            int pick = rand() % numActiveCols;
            grid.cells[numRows - 1][activeCols[pick]].isDragonLair = true;
        }

        // TODO: Initialize room metadata here (monsters, room type, etc.)
    }

    // Randomize hasTorch for every active cell (determines doorwall1 vs doorwall2
    // on the wall to the right of that door).  Far-right column cells are also
    // randomized even though their wall is never rendered.
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 3; c++) {
            if (grid.cells[r][c].state != DoorState::DOOR_NONE) {
                grid.cells[r][c].hasTorch = (rand() % 2 == 1);
            }
        }
    }

    // Signal the render function to restart the zoom-in animation
    m_dungeonGridAnimPending = true;
}

// ========================================================================
// PBTBL_INTOWER: Dungeon Grid Render
// ========================================================================

// Render the dungeon door grid for the current player.
//   scale   : final/target scale factor (1.0 = native image size)
//   centerX : screen X of the grid centre
//   centerY : screen Y of the grid centre
//   animate : if true, zoom from 0 to scale over 2 s; reset triggered by
//             pbeInitDungeonGrid setting m_dungeonGridAnimPending
void PBEngine::pbeRenderDungeonGrid(float scale, int centerX, int centerY,
                                    bool animate,
                                    unsigned long currentTick, unsigned long /*lastTick*/) {

    TowerDungeonGrid& grid = m_playerStates[m_currentPlayer].dungeonGrid;

    // ---- Animated scale ------------------------------------------------
    // When animate=true, pbeInitDungeonGrid sets m_dungeonGridAnimPending.
    // The first render call after that latches the start tick and the grid
    // zooms from scale=0 to the target scale over 2000 ms.
    float renderScale = scale;
    if (animate) {
        if (m_dungeonGridAnimPending) {
            m_dungeonGridAnimStartTick = currentTick;
            m_dungeonGridAnimPending   = false;
        }
        float t = (currentTick >= m_dungeonGridAnimStartTick)
                  ? (float)(currentTick - m_dungeonGridAnimStartTick) / 1000.0f
                  : 0.0f;
        if (t > 1.0f) t = 1.0f;

        // Ease-out quadratic: scale grows fast early so the grid is nearly
        // full-size well before it arrives — only a small pop at the end.
        float tScale    = 1.0f - (1.0f - t) * (1.0f - t);
        renderScale     = scale * tScale;

        // Swirl path: begins at the bottom-centre door position and spirals
        // CCW into the final centre over 1 second.
        //   bottomOffset = distance from grid centre to row-0 (bottom door)
        //   using final scale so the start lines up with the door's real position.
        float finalDoorH   = (float)gfxGetBaseHeight(m_DoorOpenId) * scale;
        float finalRowStep = finalDoorH * 1.25f;          // doorH + doorH/4
        float bottomOffset = 2.0f * finalRowStep;         // 2 rows below centre

        float offsetMag = (1.0f - t) * bottomOffset;
        // CCW revolution: PI/2 (below) -> PI/2 + 2PI
        float angle = 1.5707963f + t * 6.2831853f;
        float rawX  = cosf(angle) * offsetMag * 0.4f;    // Narrow left swing to ~40% of orbit radius
        float rawY  = sinf(angle) * offsetMag;

        // Never shift right of the final position — keeps the grid inside the tower image
        float startShiftX = (1.0f - t) * 30.0f;  // +30 px right at t=0, fades to 0 at t=1
        float startShiftY = (1.0f - t) * 20.0f;  // +20 px down  at t=0, fades to 0 at t=1
        centerX += (int)(rawX < 0.0f ? rawX : 0.0f) + (int)startShiftX;
        centerY += (int)rawY + (int)startShiftY;
    }

    // ---- Layout metrics (all use renderScale for consistent zoom) -------
    int doorW  = (int)(gfxGetBaseWidth(m_DoorOpenId)  * renderScale);
    int doorH  = (int)(gfxGetBaseHeight(m_DoorOpenId) * renderScale);

    int gapH    = doorH / 4;
    int rowStep = doorH + gapH;

    int wallNativeW  = (int)gfxGetBaseWidth(m_DoorWall1Id);
    int colGap       = (int)(wallNativeW * renderScale / 4.0f);
    int colStep      = doorW + colGap;

    // ---- Row positions (centred at centerY) ----------------------------
    int rowPositions[5];
    int gridTop = centerY - (2 * rowStep);
    for (int r = 0; r < 5; r++) {
        rowPositions[r] = gridTop + (4 - r) * rowStep;
    }

    // ---- Column positions (smart centering for even / odd active cols) --
    // Scan active column range so the grid is always centred at centerX
    // regardless of whether an even or odd number of columns are in use.
    //   Odd  (1 or 3 cols): centre lands on the middle column.
    //   Even (2 cols)     : centre sits in the gap between the two columns.
    int minActiveCol = 3, maxActiveCol = -1;
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 3; c++) {
            if (grid.cells[r][c].state != DoorState::DOOR_NONE) {
                if (c < minActiveCol) minActiveCol = c;
                if (c > maxActiveCol) maxActiveCol = c;
            }
        }
    }
    if (maxActiveCol < 0) { minActiveCol = 1; maxActiveCol = 1; }
    float centerColIdx = (minActiveCol + maxActiveCol) / 2.0f;

    int colPositions[3];
    for (int c = 0; c < 3; c++) {
        colPositions[c] = centerX + (int)((c - centerColIdx) * colStep);
    }

    // ---- Pass 1: Walls (lowest layer) ----------------------------------
    // Render wall horizontally between adjacent door columns on the same row.
    // Use doorwall1 (torch) if the left door has hasTorch set, else doorwall2.
    // Shifted 5 pixels left from the exact midpoint.
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 2; c++) {
            if (grid.cells[r][c].state     != DoorState::DOOR_NONE &&
                grid.cells[r][c + 1].state != DoorState::DOOR_NONE) {
                int wallX = (colPositions[c] + colPositions[c + 1]) / 2 - 5;
                unsigned int wallSprite = grid.cells[r][c].hasTorch ? m_DoorWall1Id : m_DoorWall2Id;
                gfxRenderSprite(wallSprite, wallX, rowPositions[r], renderScale, 0.0f);
            }
        }
    }

    // ---- Pass 2: Blocked overlays --------------------------------------
    // Render doorblocked.png behind every open door at 0.8x scale.
    // Both sprites use centre origin so the same colPositions[c] centres them.
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 3; c++) {
            if (grid.cells[r][c].state == DoorState::DOOR_OPEN) {
                gfxRenderSprite(m_DoorBlockedId,
                                colPositions[c],
                                rowPositions[r],
                                renderScale * 0.8f, 0.0f);
            }
        }
    }

    // ---- Pass 3: Ladders -----------------------------------------------
    // Only draw a ladder when the door below it is OPEN (reveals the path up).
    // Scale at 1.08x (20% up, then 10% back down) and anchor the top of the
    // ladder (shift centre down by 10% of the base scaled height to compensate).
    // Shifted 1 pixel to the right.
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 3; c++) {
            if (grid.cells[r][c].hasLadder &&
                grid.cells[r][c].state == DoorState::DOOR_OPEN &&
                grid.cells[r + 1][c].state != DoorState::DOOR_NONE) {
                int ladderY      = (rowPositions[r] + rowPositions[r + 1]) / 2;
                float ladderScale = renderScale * 1.08f;
                int topAnchorAdj  = (int)(gfxGetBaseHeight(m_DoorLadderId) * renderScale * 0.1f);
                gfxRenderSprite(m_DoorLadderId, colPositions[c] + 1, ladderY + topAnchorAdj + 19, ladderScale, 0.0f);
            }
        }
    }

    // ---- Pass 4: Doors (top layer) -------------------------------------
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 3; c++) {
            if (grid.cells[r][c].state == DoorState::DOOR_NONE) continue;
            unsigned int spriteId = (grid.cells[r][c].state == DoorState::DOOR_OPEN)
                                    ? m_DoorOpenId : m_DoorClosedId;
            gfxRenderSprite(spriteId, colPositions[c], rowPositions[r], renderScale, 0.0f);
        }
    }
}

// ========================================================================
// PBTBL_INTOWER: Render Function
// ========================================================================

bool PBEngine::pbeRenderInTower(unsigned long currentTick, unsigned long lastTick) {

    if (!pbeLoadInTower()) {
        pbeSendConsole("ERROR: Failed to load InTower screen resources");
        return (false);
    }

    // Render the standard main screen base: black background, player scores,
    // status text and icons, NeoPixel animation
    if (!pbeRenderMainScreenBase(currentTick, lastTick)) {
        return (false);
    }

    // Render towerclimb image centered in the main score area
    gfxRenderSprite(m_TowerClimbId, ACTIVEDISPX + (1024 / 3), ACTIVEDISPY + 350);

    // Render dungeon door grid in the right half of the towerclimb image area
    {
        int tcCenterX    = ACTIVEDISPX + (1024 / 3);
        int tcHalfWidth  = (int)(gfxGetBaseWidth(m_TowerClimbId) / 2);
        int gridCenterX  = tcCenterX + tcHalfWidth / 2 + 10;
        int gridCenterY  = ACTIVEDISPY + 350;
        pbeRenderDungeonGrid(0.65f, gridCenterX, gridCenterY, true, currentTick, lastTick);
    }

    // Render divider bars on top, same as pbeRenderMainScreen
    gfxRenderSprite(m_PBTBLMainScreenBGId, ACTIVEDISPX, ACTIVEDISPY);

    // Re-render ball text and status text above the tower image and overlay
    pbeRenderStatusText(currentTick, lastTick);

    return (true);
}

// ========================================================================
// PBTBL_INTOWER: State Update Function
// ========================================================================

void PBEngine::pbeUpdateStateInTower(stInputMessage inputMessage) {

    if (inputMessage.inputMsg == PB_IMSG_BUTTON && inputMessage.inputState == PB_ON) {

        // ------------------------------------------------------------
        // Flippers: open the next available (non-NONE, non-OPEN) door
        // in row-major order across the current player's dungeon grid
        // ------------------------------------------------------------
        if (inputMessage.inputId == IDI_LFLIP || inputMessage.inputId == IDI_RFLIP) {
            TowerDungeonGrid& grid = m_playerStates[m_currentPlayer].dungeonGrid;
            bool opened = false;
            for (int r = 0; r < 5 && !opened; r++) {
                for (int c = 0; c < 3 && !opened; c++) {
                    if (grid.cells[r][c].state != DoorState::DOOR_NONE &&
                        grid.cells[r][c].state != DoorState::DOOR_OPEN) {
                        grid.cells[r][c].state = DoorState::DOOR_OPEN;
                        opened = true;
                    }
                }
            }
        }

        // ------------------------------------------------------------
        // Activate buttons: change test dungeon level and re-initialise
        // Left activate  → level - 1 (min 1)
        // Right activate → level + 1 (max 3)
        // ------------------------------------------------------------
        else if (inputMessage.inputId == IDI_LACTIVATE) {
            if (s_testDungeonLevel > 1) {
                s_testDungeonLevel--;
                pbeInitDungeonGrid(m_currentPlayer, s_testDungeonLevel);
            }
        }
        else if (inputMessage.inputId == IDI_RACTIVATE) {
            if (s_testDungeonLevel < 3) {
                s_testDungeonLevel++;
                pbeInitDungeonGrid(m_currentPlayer, s_testDungeonLevel);
            }
        }
    }

    // Always run mode-system bookkeeping
    pbeUpdateModeSystem(inputMessage, GetTickCountGfx());
}
