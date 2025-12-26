#!/usr/bin/env python3
import argparse
import json
import random
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import numpy as np
from PIL import Image, ImageDraw

LABEL_ALIASES = {
    'body': 'body',
    'torso': 'body',
    'head': 'head',
    'hand': 'hand',
    'hands': 'hand',
}

def normalize_label(s: str) -> Optional[str]:
    if not s:
        return None
    return LABEL_ALIASES.get(s.strip().lower())

def find_labeled_jsons(root_dirs: List[Path]) -> List[Path]:
    out: List[Path] = []
    for rd in root_dirs:
        if not rd.exists():
            continue
        out.extend(sorted(rd.rglob('*.json')))
    return out

def resolve_image_path(json_path: Path, image_path_field: str) -> Optional[Path]:
    jp = json_path.parent

    cand = jp / image_path_field
    if cand.exists():
        return cand

    cand2 = Path(image_path_field)
    if cand2.is_absolute() and cand2.exists():
        return cand2

    stem = Path(image_path_field).stem if image_path_field else json_path.stem
    for ext in ['.png', '.jpg', '.jpeg', '.webp', '.bmp', '.tif', '.tiff']:
        c = jp / (stem + ext)
        if c.exists():
            return c

    return None

def polygon_to_mask(size_wh: Tuple[int, int], points: List[List[float]]) -> np.ndarray:
    w, h = size_wh
    img = Image.new('L', (w, h), 0)
    draw = ImageDraw.Draw(img)

    poly = [(float(x), float(y)) for x, y in points]
    if len(poly) >= 3:
        draw.polygon(poly, outline=255, fill=255)
    return np.array(img, dtype=np.uint8)

def build_rgb_mask_from_labelme(data: Dict, json_path: Path, head_priority: bool = True) -> Image.Image:
    w = int(data.get('imageWidth', 0))
    h = int(data.get('imageHeight', 0))
    if w <= 0 or h <= 0:
        raise ValueError(f'{json_path}: missing/invalid imageWidth/imageHeight')

    body = np.zeros((h, w), dtype=np.uint8)
    head = np.zeros((h, w), dtype=np.uint8)
    hand = np.zeros((h, w), dtype=np.uint8)

    shapes = data.get('shapes', [])
    if not isinstance(shapes, list):
        raise ValueError(f'{json_path}: shapes is not a list')

    for sh in shapes:
        label = normalize_label(sh.get('label', ''))
        if label not in ('body', 'head', 'hand'):
            continue

        if sh.get('shape_type', 'polygon') != 'polygon':
            continue

        pts = sh.get('points', [])
        if not isinstance(pts, list) or len(pts) < 3:
            continue

        poly_mask = polygon_to_mask((w, h), pts)

        if label == 'body':
            body = np.maximum(body, poly_mask)

        elif label == 'head':
            head = np.maximum(head, poly_mask)
            if head_priority:
                overlap = (hand > 0) & (head > 0)
                if np.any(overlap):
                    hand[overlap] = 0

        elif label == 'hand':
            if head_priority:
                poly_mask = np.where(head > 0, 0, poly_mask).astype(np.uint8)
                hand = np.maximum(hand, poly_mask)
            else:
                hand = np.maximum(hand, poly_mask)
                overlap = (hand > 0) & (head > 0)
                if np.any(overlap):
                    head[overlap] = 0

    rgb = np.zeros((h, w, 3), dtype=np.uint8)
    rgb[..., 0] = head   # R
    rgb[..., 1] = hand   # G
    rgb[..., 2] = body   # B
    return Image.fromarray(rgb, mode='RGB')

def write_split(
    items: List[Tuple[Path, Path]],
    out_dir: Path,
    head_priority: bool,
    dry_run: bool,
    start_index: int = 0,
) -> int:
    idx = start_index
    skipped = 0

    for jp, img_path in items:
        try:
            with jp.open('r', encoding='utf-8') as f:
                data = json.load(f)

            img = Image.open(img_path).convert('RGB')

            # Prefer actual image dimensions if JSON disagrees
            wj = int(data.get('imageWidth', img.width))
            hj = int(data.get('imageHeight', img.height))
            if (img.width, img.height) != (wj, hj):
                data['imageWidth'] = img.width
                data['imageHeight'] = img.height

            mask = build_rgb_mask_from_labelme(data, jp, head_priority=head_priority)

            base = f'{idx:04d}.png'
            base_mask = f'{idx:04d}_mask.png'
            dst_img = out_dir / base
            dst_mask = out_dir / base_mask

            if dry_run:
                print(f'[dry] {img_path} + {jp} -> {dst_img}, {dst_mask}')
            else:
                img.save(dst_img, format='PNG')
                mask.save(dst_mask, format='PNG')

            idx += 1

        except Exception as e:
            print(f'[skip] {jp}: {e}')
            skipped += 1

    return skipped

def main() -> int:
    ap = argparse.ArgumentParser(description='Combine LabelMe-labeled infant segmentation dataset into train/val folders.')
    ap.add_argument('--inputs', nargs='+',
                    default=['real_camera_baby_doll', 'stable_diffusion_1', 'stable_diffusion_2'],
                    help='Input root directories to scan.')
    ap.add_argument('--out', default='out', help='Output directory root.')
    ap.add_argument('--val-count', type=int, default=8, help='Number of validation images to hold out (default: 8).')
    ap.add_argument('--seed', type=int, default=1337, help='Shuffle RNG seed (default: 1337).')
    ap.add_argument('--head-priority', action='store_true', default=True,
                    help='Head wins over hands on overlap (default behavior).')
    ap.add_argument('--hand-priority', action='store_true',
                    help='Hands win over head on overlap.')
    ap.add_argument('--dry-run', action='store_true', help='Only report what would be done.')
    args = ap.parse_args()

    head_priority = True
    if args.hand_priority:
        head_priority = False

    in_dirs = [Path(p) for p in args.inputs]
    out_root = Path(args.out)
    train_dir = out_root / 'train'
    val_dir = out_root / 'val'

    if not args.dry_run:
        train_dir.mkdir(parents=True, exist_ok=True)
        val_dir.mkdir(parents=True, exist_ok=True)

    jsons = find_labeled_jsons(in_dirs)
    if not jsons:
        print('No JSON labels found.')
        return 1

    pairs: List[Tuple[Path, Path]] = []
    for jp in jsons:
        try:
            with jp.open('r', encoding='utf-8') as f:
                data = json.load(f)
            img_field = data.get('imagePath', '')
            img_path = resolve_image_path(jp, img_field)
            if img_path is None or not img_path.exists():
                print(f'[skip] {jp}: cannot resolve imagePath={img_field!r}')
                continue
            pairs.append((jp, img_path))
        except Exception as e:
            print(f'[skip] {jp}: {e}')

    if not pairs:
        print('No usable (json, image) pairs found.')
        return 1

    rng = random.Random(args.seed)
    rng.shuffle(pairs)

    val_n = min(args.val_count, len(pairs))
    val_items = pairs[:val_n]
    train_items = pairs[val_n:]

    print(f'Total labeled samples: {len(pairs)}')
    print(f'Validation: {len(val_items)}')
    print(f'Training: {len(train_items)}')
    print(f'Output: {out_root}')

    skipped_train = write_split(train_items, train_dir, head_priority=head_priority, dry_run=args.dry_run, start_index=0)
    skipped_val = write_split(val_items, val_dir, head_priority=head_priority, dry_run=args.dry_run, start_index=0)

    print(f'Done. Skipped train={skipped_train}, val={skipped_val}.')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())

