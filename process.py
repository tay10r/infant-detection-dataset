import json
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

import numpy as np
from PIL import Image, ImageDraw

def find_labeled_jsons(root_dirs: list[Path]) -> list[Path]:
    out: list[Path] = []
    for rd in root_dirs:
        if not rd.exists():
            continue
        out.extend(sorted(rd.rglob('*.json')))
    return out

def resolve_image_path(json_path: Path, image_path_field: str) -> Path | None:
    jp = json_path.parent

    cand = jp / image_path_field
    if image_path_field and cand.exists():
        return cand

    cand2 = Path(image_path_field) if image_path_field else None
    if cand2 and cand2.is_absolute() and cand2.exists():
        return cand2

    stem = Path(image_path_field).stem if image_path_field else json_path.stem
    for ext in ['.png']:
        c = jp / (stem + ext)
        if c.exists():
            return c

    return None

def polygon_to_mask(size_wh: tuple[int, int], points: list[list[float]]) -> np.ndarray:
    w, h = size_wh
    img = Image.new('L', (w, h), 0)
    draw = ImageDraw.Draw(img)

    poly = [(float(x), float(y)) for x, y in points]
    if len(poly) >= 3:
        draw.polygon(poly, outline=255, fill=255)

    return np.array(img, dtype=np.uint8)

def build_baby_mask_from_labelme(data: dict, json_path: Path) -> np.ndarray:
    w = int(data.get('imageWidth', 0))
    h = int(data.get('imageHeight', 0))
    if w <= 0 or h <= 0:
        raise ValueError(f'{json_path}: missing/invalid imageWidth/imageHeight')

    baby = np.zeros((h, w), dtype=np.uint8)

    shapes = data.get('shapes', [])
    if not isinstance(shapes, list):
        raise ValueError(f'{json_path}: shapes is not a list')

    for sh in shapes:
        label = sh.get('label', '')
        if label != 'baby':
            continue

        if sh.get('shape_type', 'polygon') != 'polygon':
            continue

        pts = sh.get('points', [])
        if not isinstance(pts, list) or len(pts) < 3:
            continue

        poly_mask = polygon_to_mask((w, h), pts)  # 0/255
        # accumulate as 0/1
        baby = np.maximum(baby, (poly_mask > 0).astype(np.uint8))

    # ensure strictly {0,1}
    baby = np.where(baby > 0, 1, 0).astype(np.uint8)
    return baby

def load_pairs_from_dirs(root_dirs: list[Path]) -> list[tuple[Path, Path]]:
    jsons = find_labeled_jsons(root_dirs)
    pairs: list[tuple[Path, Path]] = []

    for jp in jsons:
        try:
            with jp.open('r', encoding="utf-8") as f:
                data = json.load(f)

            img_field = data.get('imagePath', "")
            img_path = resolve_image_path(jp, img_field)
            if img_path is None or not img_path.exists():
                print(f'[skip] {jp}: cannot resolve imagePath={img_field!r}')
                continue

            pairs.append((jp, img_path))
        except Exception as e:
            print(f'[skip] {jp}: {e}')

    return pairs

def process(dirs: list[Path], output_prefix: str) -> int:
    pairs = load_pairs_from_dirs(dirs)
    if not pairs:
        raise RuntimeError('No usable (json, image) pairs found.')

    pairs.sort(key=lambda t: str(t[0]))

    n = len(pairs)
    H = 768
    W = 768

    images = np.empty((n, 3, H, W), dtype=np.uint8) # NCHW
    masks  = np.empty((n, 1, H, W), dtype=np.uint8) # N1HW

    skipped = 0
    write_i = 0

    for jp, img_path in pairs:
        try:
            with jp.open('r', encoding='utf-8') as f:
                data = json.load(f)

            img = Image.open(img_path).convert('RGB')
            if img.size != (768, 768):
                assert img.size[0] == img.size[1]
                img = img.resize((768, 768))

            # Prefer actual image dimensions if JSON disagrees
            if int(data.get('imageWidth', img.width)) != img.width or int(data.get('imageHeight', img.height)) != img.height:
                data['imageWidth'] = img.width
                data['imageHeight'] = img.height

            baby_mask_hw = build_baby_mask_from_labelme(data, jp)  # (H,W) uint8 {0,1}
            if baby_mask_hw.shape != (H, W):
                raise ValueError(f'{jp}: mask shape {baby_mask_hw.shape}, expected {(H, W)}')

            img_hwc = np.asarray(img, dtype=np.uint8)  # (H,W,3)
            if img_hwc.shape != (H, W, 3):
                raise ValueError(f'{jp}: image array shape {img_hwc.shape}, expected {(H, W, 3)}')

            images[write_i] = np.transpose(img_hwc, (2, 0, 1))
            masks[write_i, 0] = baby_mask_hw

            write_i += 1

        except Exception as e:
            print(f'[skip] {jp}: {e}')
            skipped += 1

    if write_i == 0:
        raise RuntimeError('All samples were skipped.')

    if write_i != n:
        images = images[:write_i]
        masks = masks[:write_i]
        n = write_i

    Path(f'{output_prefix}.bin').write_bytes(images.tobytes(order='C'))
    Path(f'{output_prefix}_mask.bin').write_bytes(masks.tobytes(order='C'))
    return n

def main():
    train_dirs = [Path(p) for p in [ 'data/train/0', 'data/train/1', 'data/train/2' ]]
    val_dirs = [Path(p) for p in [ 'data/val' ]]
    num_train_samples = process(train_dirs, 'train')
    num_val_samples = process(val_dirs, 'val')
    with open('info.json', 'w') as f:
        f.write(json.dumps({
            'num_train_samples': num_train_samples,
            'num_val_samples': num_val_samples,
            'width': 768,
            'height': 768,
            'dtype': 'uint8'
        }))
    with ZipFile('infant-detection-dataset.zip', mode='w', compresslevel=9, compression=ZIP_DEFLATED) as z:
        z.write('train.bin')
        z.write('train_mask.bin')
        z.write('val.bin')
        z.write('val_mask.bin')
        z.write('info.json')

if __name__ == '__main__':
    main()