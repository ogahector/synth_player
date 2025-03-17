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
        int textY = y + boxHeight / 2 + 4;  // Approximate vertical centering
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
        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        sysState.joystickPress = false; // Reset joystick press
        xSemaphoreGive(sysState.mutex);
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
    int actionBoxHeight = screenHeight - 18; // leave space for the title

    // Left box: Record
    int recX = spacing;
    int recY = 18;
    if (actionSelection == 0) {
        u8g2.drawRBox(recX, recY, actionBoxWidth, actionBoxHeight, 3);
        u8g2.setDrawColor(0);
    } else {
        u8g2.drawRFrame(recX, recY, actionBoxWidth, actionBoxHeight, 3);
        u8g2.setDrawColor(1);
    }
    u8g2.drawStr(recX + 10, recY + actionBoxHeight / 2 + 4, "Record");
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
    u8g2.drawStr(playX + 10, playY + actionBoxHeight / 2 + 4, "Play");
    u8g2.setDrawColor(1);
    xSemaphoreTake(record.mutex,portMAX_DELAY);
    // Show REC
    if (record.recording){
        u8g2.setDrawColor(1);
        u8g2.drawRFrame(90, 2, 35, 12, 3);
        u8g2.drawStr(100, 12, "REC");
        u8g2.drawDisc(95, 7, 2, U8G2_DRAW_ALL);
    }
    if (record.playback){
        u8g2.setDrawColor(1);
        u8g2.drawRFrame(80, 2, 45, 12, 3);
        u8g2.drawStr(90, 12, "PLAY");
        u8g2.drawTriangle(83, 4, 83,10, 88, 7);
    }
    xSemaphoreGive(record.mutex);

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
        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        sysState.joystickPress = false; // Reset joystick press
        xSemaphoreGive(sysState.mutex);
        return actionSelection; // Confirm action selection: 0 = Record, 1 = Play.
    }

    return -1; // Not confirmed yet.
}

void renderRecording(bool alreadyShown){
    if (!alreadyShown){
        selectedTrack = -1;
        selectedAction = -1;
    }
    if (selectedTrack < 0) {
        selectedTrack = renderTrackSelection();
    }

        // --- Track Action Selection Screen ---
       
    if (selectedTrack>=0){
        selectedAction = renderTrackActionSelection(selectedTrack);
    }

        // --- Execute the Chosen Action ---
    if (selectedAction == 0) {
        xSemaphoreTake(record.mutex,portMAX_DELAY);
        record.recording = !record.recording;
        if (record.recording) {
            switch (selectedTrack)
            {
            case 0:
                record.active_tracks = record.active_tracks & 0b1110; // Disables current track from playback when starting recording
                break;
            case 1:
                record.active_tracks = record.active_tracks & 0b1101;
                break;
            case 2:
                record.active_tracks = record.active_tracks & 0b1011;
                break;
            case 3:
                record.active_tracks = record.active_tracks & 0b0111;
                break;
            default:
                break;
            }
        }

        // record.playback = false; // Uncomment if you want to pause playback while recording
        record.current_track = selectedTrack;
        xSemaphoreGive(record.mutex);
    } else if (selectedAction == 1) {
        xSemaphoreTake(record.mutex,portMAX_DELAY);
        record.recording = false; //Disable recording
        switch (selectedTrack)
        {
        case 0:
            record.active_tracks = record.active_tracks ^ 0b0001; // XOR of active tracks, toggles the track being played
            break;
        case 1:
            record.active_tracks = record.active_tracks ^ 0b0010;
            break;
        case 2:
            record.active_tracks = record.active_tracks ^ 0b0100;
            break;
        case 3:
            record.active_tracks = record.active_tracks ^ 0b1000;
            break;
        default:
            break;
        }
        if (record.active_tracks == 0) record.playback = false;
        else record.playback = true;
        xSemaphoreGive(record.mutex);
    }
}