import cv2
import mediapipe as mp
import time

from mediapipe.tasks import python
from mediapipe.tasks.python import vision


# ==============================
# FILES
# ==============================
MODEL_PATH = "src/hand_landmarker.task"
RPM_FILE = "rpm.txt"
ENGINE_FILE = "engine.txt"


# ==============================
# ENGINE VARIABLES
# ==============================
rpm = 3200
engine_running = True
boost_mode = False


# ==============================
# TIMING
# ==============================
last_change_time = 0
change_delay = 0.15

# Prevent repeated ONE FINGER toggles
one_finger_ready = True


# ==============================
# MEDIAPIPE
# ==============================
base_options = python.BaseOptions(
    model_asset_path=MODEL_PATH
)

options = vision.HandLandmarkerOptions(
    base_options=base_options,
    running_mode=vision.RunningMode.VIDEO,
    num_hands=1,
    min_hand_detection_confidence=0.7,
    min_hand_presence_confidence=0.7,
    min_tracking_confidence=0.7
)

detector = vision.HandLandmarker.create_from_options(
    options
)


# ==============================
# CAMERA
# ==============================
cap = cv2.VideoCapture(0)

if not cap.isOpened():
    print("Camera could not be opened.")
    detector.close()
    exit()


# ==============================
# INITIAL FILE VALUES
# ==============================
with open(RPM_FILE, "w") as file:
    file.write(str(rpm))

with open(ENGINE_FILE, "w") as file:
    file.write("1")


# ==============================
# MAIN LOOP
# ==============================
while True:

    success, frame = cap.read()

    if not success:
        break

    frame = cv2.flip(frame, 1)

    rgb = cv2.cvtColor(
        frame,
        cv2.COLOR_BGR2RGB
    )

    mp_image = mp.Image(
        image_format=mp.ImageFormat.SRGB,
        data=rgb
    )

    timestamp_ms = int(
        cv2.getTickCount()
        / cv2.getTickFrequency()
        * 1000
    )

    result = detector.detect_for_video(
        mp_image,
        timestamp_ms
    )

    gesture = "NO HAND"

    # ==============================
    # HAND DETECTED
    # ==============================
    if result.hand_landmarks:

        hand = result.hand_landmarks[0]

        tips = [8, 12, 16, 20]
        pips = [6, 10, 14, 18]

        fingers_up = 0

        for tip, pip in zip(tips, pips):

            if hand[tip].y < hand[pip].y:
                fingers_up += 1


        # ==============================
        # GESTURE DETECTION
        # ==============================
        if fingers_up == 0:

            gesture = "FIST"

        elif fingers_up == 1:

            gesture = "ONE FINGER"

        elif fingers_up == 2:

            gesture = "TWO FINGERS"

        elif fingers_up == 3:

            gesture = "THREE FINGERS"

        elif fingers_up == 4:

            gesture = "OPEN HAND"

        else:

            gesture = "OTHER"


        current_time = time.time()


        # ==============================
        # ONE FINGER = START / STOP
        # ==============================
        if gesture == "ONE FINGER":

            if one_finger_ready:

                engine_running = not engine_running

                with open(ENGINE_FILE, "w") as file:

                    file.write(
                        "1"
                        if engine_running
                        else "0"
                    )

                one_finger_ready = False


        else:

            # Finger must be removed
            # before another toggle
            one_finger_ready = True


        # ==============================
        # BOOST MODE
        # ==============================
        if gesture == "TWO FINGERS":

            boost_mode = True

        elif gesture == "THREE FINGERS":

            boost_mode = False


        # ==============================
        # RPM CONTROL
        # ==============================
        if engine_running:

            if (
                current_time - last_change_time
                >= change_delay
            ):

                if gesture == "OPEN HAND":

                    rpm += 10

                    last_change_time = current_time


                elif gesture == "FIST":

                    rpm -= 10

                    last_change_time = current_time


                elif gesture == "TWO FINGERS":

                    rpm += 50

                    last_change_time = current_time


        # ==============================
        # RPM LIMIT
        # ==============================
        rpm = max(
            1000,
            min(10000, rpm)
        )


        # ==============================
        # WRITE RPM
        # ==============================
        with open(RPM_FILE, "w") as file:

            file.write(str(rpm))


        # ==============================
        # DRAW LANDMARKS
        # ==============================
        for landmark in hand:

            x = int(
                landmark.x *
                frame.shape[1]
            )

            y = int(
                landmark.y *
                frame.shape[0]
            )

            cv2.circle(
                frame,
                (x, y),
                5,
                (0, 255, 0),
                -1
            )


    # ==============================
    # STATUS
    # ==============================
    status = (
        "RUNNING"
        if engine_running
        else "STOPPED"
    )

    mode = (
        "BOOST MODE"
        if boost_mode
        else "NORMAL MODE"
    )


    # ==============================
    # CAMERA UI
    # ==============================
    cv2.putText(
        frame,
        "TURBOFAN HAND CONTROL",
        (30, 45),
        cv2.FONT_HERSHEY_SIMPLEX,
        1,
        (0, 255, 255),
        2
    )

    cv2.putText(
        frame,
        "Gesture: " + gesture,
        (30, 90),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (0, 255, 0),
        2
    )

    cv2.putText(
        frame,
        f"RPM: {rpm}",
        (30, 135),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (255, 255, 0),
        2
    )

    cv2.putText(
        frame,
        "Engine: " + status,
        (30, 175),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (0, 255, 0)
        if engine_running
        else (0, 0, 255),
        2
    )

    cv2.putText(
        frame,
        "Mode: " + mode,
        (30, 215),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (255, 150, 0),
        2
    )

    cv2.putText(
        frame,
        "OPEN HAND = +10 RPM",
        (30, 260),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (255, 255, 255),
        2
    )

    cv2.putText(
        frame,
        "FIST = -10 RPM",
        (30, 295),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (255, 255, 255),
        2
    )

    cv2.putText(
        frame,
        "ONE FINGER = START / STOP",
        (30, 330),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (255, 255, 255),
        2
    )

    cv2.putText(
        frame,
        "TWO FINGERS = BOOST (+50 RPM)",
        (30, 365),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (255, 200, 0),
        2
    )

    cv2.putText(
        frame,
        "THREE FINGERS = NORMAL MODE",
        (30, 400),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (255, 255, 255),
        2
    )

    cv2.putText(
        frame,
        "Q = EXIT",
        (30, 435),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (200, 200, 200),
        2
    )


    # ==============================
    # SHOW CAMERA
    # ==============================
    cv2.imshow(
        "Turbofan - Hand Control",
        frame
    )


    # ==============================
    # EXIT
    # ==============================
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break


# ==============================
# CLEANUP
# ==============================
cap.release()

detector.close()

cv2.destroyAllWindows()