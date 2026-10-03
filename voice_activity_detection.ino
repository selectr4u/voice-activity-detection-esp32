#include <M5Unified.h>
#include <cmath>

// constants for weights, certainty thresholds and general constants
static constexpr float RMSE_W = 0.25f;
static constexpr float ZCR_W = 0.25f;
static constexpr float PP_W = 0.5f; // pitch score is our most important
static constexpr float CERTAINTY_THRESHOLD = 0.6f;
static constexpr size_t FRAME_LEN = 480; // 480 PCM samples is 30ms frame window at 16 kHz (since mic records at 16 kHz)
static constexpr float RMSE_CEILING = 8000;
static constexpr float RMSE_FLOOR = 100;
static constexpr float RELEASE_THRESHOLD = 0.4f; // certainty threshold but while we are speaking!

// the buffer for the pcm data
static int16_t pcm_buffer[FRAME_LEN];

// buffer for averages
static float score_history[3] = {0.0f, 0.0f, 0.0f};
static size_t score_history_idx = 0; // counter, goes 0 to 4 and returns back!

// boolean to determine if we are actively speaking
static bool is_speaking = false;

// extraction functions
float extract_rmse_score(const int16_t* pcm_data, size_t len) {
  float sum_of_squares = 0;
  float raw_rmse;
  float normalised_rmse;

  for (size_t i = 0; i < len; i++) {
    sum_of_squares += (float)pcm_data[i] * (float)pcm_data[i];
  }

  raw_rmse = sqrtf((sum_of_squares / len));

  // to normalise and make sure entire use of scale we clamp
  normalised_rmse = (raw_rmse - RMSE_FLOOR) / (RMSE_CEILING-RMSE_FLOOR);

  if (normalised_rmse < 0.0f) {
    normalised_rmse = 0.0f;
  } else if (normalised_rmse > 1.0f) {
    normalised_rmse = 1.0f;
  }

  return normalised_rmse;
}

float extract_zcr_score(const int16_t* pcm_data, size_t len) {
  int16_t crossings = 0;
  float normalised_zcr;

  for (size_t i = 1; i <= len-1; i++) {
    if ((pcm_data[i] >= 0 && pcm_data[i-1] < 0) || (pcm_data[i] < 0 && pcm_data[i-1] >=0 )) {
      crossings++;
    }
  }

  normalised_zcr = (float)crossings/((float)(len-1));

  return (float)(1.0f-normalised_zcr); // because lower ZCR = higher certainty
}

float extract_pitch_score(const int16_t* pcm_data, size_t len) {
  // O(n^2) most likely since two loops in another

  float lowest_amdf = 1e9f; // large number thats unachievable in reality so we can compare

  // iterate through each 'lag' (lag is basically like an interval)
  // originally 35 and 200, but lets account for higher freqs?
  for (size_t i=30; i <= 210; i++) {
    int16_t count = 0;
    float sum = 0.0f;
    float current_amdf;
    for (size_t j=0; (j+i) < len; j++) {
      count++;
      sum += abs(pcm_data[j] - pcm_data[j+i]);
    }

    current_amdf = (sum/(float)count);
    if (current_amdf < lowest_amdf) {
      lowest_amdf = current_amdf;
    }
  }

  float average_magnitude;
  float sum = 0;

  for (size_t i=0; i<len; i++) {
    sum += abs(pcm_data[i]);
  }

  average_magnitude = (sum / (float)len);

  // normalise against baseline
  float normalised_pitch_score;

  normalised_pitch_score = (lowest_amdf/average_magnitude);
  if (normalised_pitch_score > 1.0f) {
    normalised_pitch_score = 1.0f;
  } else if (normalised_pitch_score < 0.0f) {
    normalised_pitch_score = 0.0f;
  }

  return (1.0 - normalised_pitch_score);
}

// the main loop and setup

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(9600); // standard 9600 baud

  // microphone config
  M5.Speaker.end();
  M5.Mic.begin();
}

void loop() {
  M5.update();

  if (M5.Mic.isEnabled() && M5.Mic.record(pcm_buffer, FRAME_LEN)) {
    float rmse_f = extract_rmse_score(pcm_buffer, FRAME_LEN);
    float zcr_f = extract_zcr_score(pcm_buffer, FRAME_LEN);
    float pp_f = extract_pitch_score(pcm_buffer, FRAME_LEN);

    // calculate our certainty score
    float certainty = (RMSE_W * rmse_f) + (ZCR_W * zcr_f) + (PP_W * pp_f);

    score_history[score_history_idx] = certainty;

    Serial.printf("30MS FRAME: [CERTAINTY = %.2f] [RMSE = %.2f], [ZCR = %.2f], [PP = %.2f] \n", certainty, rmse_f, zcr_f, pp_f);

    // calculate our averages and increment score history index as appropiate
    score_history_idx = ((score_history_idx + 1) % 3); // ensures it doesnt go beyond the buffer
    float sum = 0.0f; 
    for (size_t i = 0; i < 3; i++) {
      sum += score_history[i];
    }

    float average_certainty = (sum / 3.0f);

    if (!is_speaking) {
      if (average_certainty >= CERTAINTY_THRESHOLD) {
        is_speaking = true;
      }
    } else {
      if (average_certainty < RELEASE_THRESHOLD) {
        is_speaking = false;
      }
    }

    if (is_speaking) {
      // speech detected
      Serial.printf("VOICE DETECTED BASED ON AVERAGE CERTAINTY OF %.2f\n", average_certainty);
      M5.Display.fillRect(0, 0, M5.Display.width(), 50, GREEN);
      M5.Display.setTextColor(BLACK);
      M5.Display.drawString("voice detected", 5, 5);
    } else {
      // not speech
      M5.Display.fillRect(0, 0, M5.Display.width(), 50, BLACK);
      M5.Display.setTextColor(WHITE);
      M5.Display.drawString("listening...", 5,5);
    }
  }
 
  delay(30);
}
