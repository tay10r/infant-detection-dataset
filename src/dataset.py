import json
from pathlib import Path
from random import Random

import numpy as np

from torch import Tensor, from_numpy, nn
from torch.utils.data import Dataset as DatasetBase

from torchvision.transforms.v2 import functional as FT, InterpolationMode

class AugmentationPipeline(nn.Module):
    def __init__(self, seed: int = 0):
        super().__init__()
        self.__rng = Random(seed)

    def forward(self, x: Tensor, y: Tensor) -> tuple[Tensor, Tensor]:
        if self.__rng.randint(0, 1):
            x = FT.adjust_brightness(x, brightness_factor=self.__rng.uniform(0.1, 1.5))
        if self.__rng.randint(0, 1):
            x = FT.adjust_gamma(x, gamma=self.__rng.uniform(0.1, 3.0))
        if self.__rng.randint(0, 1):
            x = FT.gaussian_noise(x, sigma=self.__rng.uniform(0.01, 0.1))
        if self.__rng.randint(0, 1):
            x = FT.gaussian_blur(x, kernel_size=[9, 9], sigma=[2, 2])
        if self.__rng.randint(0, 1):
            x = FT.jpeg(x, quality=self.__rng.randint(30, 90))
        if self.__rng.randint(0, 1):
            x = FT.rgb_to_grayscale(x, num_output_channels=3)
        angle = self.__rng.uniform(0, 359)
        x = FT.rotate_image(x, angle=angle, interpolation=InterpolationMode.BILINEAR)
        y = FT.rotate_image(y, angle=angle, interpolation=InterpolationMode.NEAREST)
        return x, y

class Dataset(DatasetBase):
    def __init__(self, root: Path, kind: str, mean: list[float], std: list[float], augmentation: nn.Module | None = AugmentationPipeline(seed=0), mask_resize: tuple[int, int] | None = None):
        with open(root / 'info.json') as f:
            info = json.load(f)
        self.__num_samples = info[f'num_{kind}_samples']
        self.__width = info['width']
        self.__height = info['height']
        rgb_shape = (self.__num_samples, 3, self.__height, self.__width)
        mask_shape = (self.__num_samples, 1, self.__height, self.__width)
        self.__images = np.memmap(str(root / f'{kind}.bin'), dtype=np.uint8, mode='r', shape=rgb_shape)
        self.__mask = np.memmap(str(root / f'{kind}_mask.bin'), dtype=np.uint8, mode='r', shape=mask_shape)
        self.__mask_resize = mask_resize
        self.__augmentation = augmentation
        self.__mean = mean
        self.__std = std

    def __len__(self) -> int:
        return self.__num_samples

    def __getitem__(self, index: int) -> tuple[Tensor, Tensor]:
        img = from_numpy(self.__images[index])
        mask = from_numpy(self.__mask[index])
        if self.__augmentation:
            img, mask = self.__augmentation(img, mask)
        if self.__mask_resize is not None:
            h = self.__mask_resize[0]
            w = self.__mask_resize[1]
            mask = FT.resize_image(mask, size=[h, w], interpolation=FT.InterpolationMode.NEAREST)
        img = img.float() * (1.0 / 255.0)
        img = FT.normalize(img, self.__mean, self.__std)
        return (img, mask)