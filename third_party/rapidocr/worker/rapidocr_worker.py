import argparse
import json
import sys
from pathlib import Path

import cv2
import numpy as np
from rapidocr import RapidOCR


def build_engine():
    return RapidOCR(
        params={
            "Global.log_level": "error",
            "Global.text_score": 0.25,
            "Global.use_cls": False,
        }
    )


ENGINE = build_engine()


def preprocess_for_red_text(image_bgr):
    hsv = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2HSV)

    lower_red_1 = np.array([0, 70, 45], dtype=np.uint8)
    upper_red_1 = np.array([18, 255, 255], dtype=np.uint8)
    lower_red_2 = np.array([160, 70, 45], dtype=np.uint8)
    upper_red_2 = np.array([180, 255, 255], dtype=np.uint8)

    mask = cv2.inRange(hsv, lower_red_1, upper_red_1)
    mask |= cv2.inRange(hsv, lower_red_2, upper_red_2)

    kernel_close = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
    kernel_dilate = cv2.getStructuringElement(cv2.MORPH_RECT, (2, 2))
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel_close, iterations=2)
    mask = cv2.dilate(mask, kernel_dilate, iterations=1)

    points = cv2.findNonZero(mask)
    if points is None:
        return None

    x, y, w, h = cv2.boundingRect(points)
    margin = 6
    x = max(0, x - margin)
    y = max(0, y - margin)
    w = min(image_bgr.shape[1] - x, w + margin * 2)
    h = min(image_bgr.shape[0] - y, h + margin * 2)
    cropped_image = image_bgr[y : y + h, x : x + w]
    cropped_mask = mask[y : y + h, x : x + w]

    canvas = np.full_like(cropped_image, 255)
    canvas[cropped_mask > 0] = cropped_image[cropped_mask > 0]

    scale = 3
    enlarged = cv2.resize(canvas, None, fx=scale, fy=scale, interpolation=cv2.INTER_CUBIC)
    return cv2.GaussianBlur(enlarged, (3, 3), 0)


def run_ocr(image_path):
    image_bgr = cv2.imread(str(image_path))
    if image_bgr is None:
        return {"ok": False, "error": "无法读取截图文件。"}

    processed = preprocess_for_red_text(image_bgr)
    if processed is None:
        return {"ok": False, "error": "没有检测到明显的红色文字区域。"}

    result = ENGINE(processed)
    texts = [text.strip() for text in getattr(result, "txts", []) if text and text.strip()]
    scores = [float(score) for score in getattr(result, "scores", [])]

    if not texts:
        return {"ok": False, "error": "OCR 没有识别出可用文字。"}

    average_score = sum(scores) / len(scores) if scores else 0.0
    return {
        "ok": True,
        "text": "\n".join(texts),
        "lines": texts,
        "averageScore": average_score,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--image", required=True)
    args = parser.parse_args()

    payload = run_ocr(Path(args.image))
    sys.stdout.buffer.write(json.dumps(payload, ensure_ascii=False).encode("utf-8"))
    sys.stdout.buffer.flush()


if __name__ == "__main__":
    main()
