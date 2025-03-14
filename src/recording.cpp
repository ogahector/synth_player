#include <globals.h>

const int boxWidth = u8g2.getDisplayWidth() / 2 - 2;
const int boxHeight = u8g2.getDisplayHeight() / 2 - 2;
const int spacing = 1;
const int margin = 2;

// Global variables to store selection indices
static int trackSelection = 0;  // For track selection (0-3)
static int actionSelection = 0; // For action selection: 0 = Record, 1 = Play

int selectedTrack = -1;
int selectedAction = -1;
// -----------------------------------------------------------------------------
// Track Selection Screen
// -----------------------------------------------------------------------------
/*
   This function displays four boxes (one per track).
   The currently highlighted track is drawn with a filled rounded box.
   It returns the track index when the joystick is pressed; otherwise, it returns -1.
*/
int renderTrackSelection() {
    u8g2.clearBuffer();

    for (int i = 0; i < 4; i++) { // 4 tracks
        int col = i % 2;
        int row = i / 2;
        int x = col * (boxWidth + spacing) + spacing;
        int y = row * (boxHeight + spacing) + spacing;

        // Highlight the selected track by drawing a filled rounded box.
        if (i == trackSelection) {
            u8g2.drawRBox(x, y, boxWidth, boxHeight, 3);
            u8g2.setDrawColor(0);  // Invert color for text
        } else {
            u8g2.drawRFrame(x, y, boxWidth, boxHeight, 3);
            u8g2.setDrawColor(1);
        }

        // Create and draw the track label ("Track 1", etc.)
        char label[10];
        sprintf(label, "Track %d", i + 1);
        int strWidth = u8g2.getStrWidth(label);
        int textX = x + (boxWidth - strWidth) / 2;
        int textY = y + boxHeight / 2;  // Approximate vertical centering
        u8g2.drawStr(textX, textY, label);
        u8g2.setDrawColor(1);
    }

    u8g2.sendBuffer();

    // Read joystick input
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    int jx = sysState.joystickHorizontalDirection;
    int jy = sysState.joystickVerticalDirection;
    bool press = sysState.joystickPress;
    xSemaphoreGive(sysState.mutex);

    // Update selection horizontally
    if (jx > 650) {  // Move left
        if (trackSelection % 2 == 1)
            trackSelection -= 1;
    } else if (jx < 300) {  // Move right
        if (trackSelection % 2 == 0)
            trackSelection += 1;
    }
    // Update selection vertically
    if (jy < 300) {  // Move up
        if (trackSelection >= 2)
            trackSelection -= 2;
    } else if (jy > 650) {  // Move down
        if (trackSelection < 2)
            trackSelection += 2;
    }

    if (press) {  // Confirm selection
        return trackSelection;
    }
    return -1; // Not confirmed yet
}

// -----------------------------------------------------------------------------
// Track Action Selection Screen
// -----------------------------------------------------------------------------
/*
   This function displays two boxes—one for "Record" and one for "Play".
   A title shows the currently selected track.
   It returns 0 for Record or 1 for Play when the joystick is pressed; otherwise, -1.
*/
int renderTrackActionSelection(int track) {
    u8g2.clearBuffer();

    // Display a title indicating the current track (e.g., "Track 1")
    char title[20];
    sprintf(title, "Track %d", track + 1);
    u8g2.drawStr(10, 12, title);

    // Calculate dimensions for the action boxes (side-by-side)
    int screenWidth = u8g2.getDisplayWidth();
    int screenHeight = u8g2.getDisplayHeight();
    int actionBoxWidth = (screenWidth - 3 * spacing) / 2;
    int actionBoxHeight = screenHeight - 20; // leave space for the title

    // Left box: Record
    int recX = spacing;
    int recY = 20;
    if (actionSelection == 0) {
        u8g2.drawRBox(recX, recY, actionBoxWidth, actionBoxHeight, 3);
        u8g2.setDrawColor(0);
    } else {
        u8g2.drawRFrame(recX, recY, actionBoxWidth, actionBoxHeight, 3);
        u8g2.setDrawColor(1);
    }
    u8g2.drawStr(recX + 10, recY + actionBoxHeight / 2, "Record");
    u8g2.setDrawColor(1);

    // Right box: Play
    int playX = recX + actionBoxWidth + spacing;
    int playY = recY;
    if (actionSelection == 1) {
        u8g2.drawRBox(playX, playY, actionBoxWidth, actionBoxHeight, 3);
        u8g2.setDrawColor(0);
    } else {
        u8g2.drawRFrame(playX, playY, actionBoxWidth, actionBoxHeight, 3);
        u8g2.setDrawColor(1);
    }
    u8g2.drawStr(playX + 10, playY + actionBoxHeight / 2, "Play");
    u8g2.setDrawColor(1);

    u8g2.sendBuffer();

    // Read joystick input for action selection
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    int jx = sysState.joystickHorizontalDirection;
    bool press = sysState.joystickPress;
    xSemaphoreGive(sysState.mutex);

    // Update the action selection based on horizontal movement.
    // Here, we simply choose Record when jx is high and Play when jx is low.
    if (jx > 650) {
        actionSelection = 0;
    } else if (jx < 300) {
        actionSelection = 1;
    }

    if (press) {
        return actionSelection; // Confirm action selection: 0 = Record, 1 = Play.
    }

    return -1; // Not confirmed yet.
}

void renderRecording(){
    
    if (selectedTrack < 0) {
        selectedTrack = renderTrackSelection();
    }

        // --- Track Action Selection Screen ---
       
    if (selectedTrack>=0 && selectedAction<0){
        selectedAction = renderTrackActionSelection(selectedTrack);
    }

        // --- Execute the Chosen Action ---
    if (selectedAction == 0) {
            // Call your recording function here to overwrite the current track.
            // For example: recordTrack(selectedTrack);
    } else if (selectedAction == 1) {
            // Call your playback function here.
            // For example: playTrack(selectedTrack);
    }
}