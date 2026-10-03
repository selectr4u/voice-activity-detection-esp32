# voice-activity-detection-esp32
Simple voice activity detection on an ESP-32 with a microphone.

This project is very simple, and was more of an investigation into how voice activity detection could be implemented on an ESP-32. It also supplemented my learning quite well, especially in computer science.

The voice activity detection is *very* primitive, with manual weights being used for each feature, and 3 features being used to determine the certainty score. This project could be expanded and drastically improved by calculating the most efficient weights for the 3 features, and making use of more features (such as Sub-band energy ratio). However, I will comment, at the time of writing I am *not* in university but will be starting soon, and A-Level Mathematics does not each us discrete Fourier transforms which would be needed for sub-band energy ratio (although I will definitely do some research with respects to that).

There can be some practical use cases for this. A very niche use case I have made note of is to use this for hard-of-hearing people, to gently vibrate and notify them when someone is trying to talk to them. Obviously, there is a lot of nuance to this in that you must account for the user themselves talking and so on, which if you were to implement a lazy solution, you could allow them to switch it off and on.
