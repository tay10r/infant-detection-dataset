from pathlib import Path

import torchvision
from torchvision.transforms.v2 import functional as FT, InterpolationMode
import torch
from torch.utils.data import DataLoader

from loguru import logger

from src.net import Net
from src.dataset import Dataset

def train_epoch(net: Net, optimizer: torch.optim.Optimizer, loader: DataLoader, device: torch.device) -> float:
    net.train()
    total_loss = 0.0
    for sample in loader:
        x: torch.Tensor = sample[0].to(device)
        y: torch.Tensor = sample[1].to(device).float()
        y_predicted = net(x)
        loss = torch.nn.functional.binary_cross_entropy_with_logits(y_predicted, y)
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()
        total_loss += loss.item()

    return total_loss / len(loader)

def validate(epoch: int, net: Net, loader: DataLoader, device: torch.device, logs_dir: Path) -> float:
    total_loss = 0.0
    total_loss = 0.0
    images: list[torch.Tensor] = []
    for sample in loader:
        x: torch.Tensor = sample[0].to(device)
        y: torch.Tensor = sample[1].to(device).float()
        y_predicted = net(x)
        loss = torch.nn.functional.binary_cross_entropy_with_logits(y_predicted, y)
        total_loss += loss.item()
        # log
        yp = FT.resize_image(y_predicted, size=[x.shape[2], x.shape[3]], interpolation=InterpolationMode.NEAREST)
        yt = FT.resize_image(y,           size=[x.shape[2], x.shape[3]], interpolation=InterpolationMode.NEAREST)
        images.append(x)
        images.append(FT.grayscale_to_rgb(yp))
        images.append(FT.grayscale_to_rgb(yt))
    g = torchvision.utils.make_grid(torch.concat(images, dim=0), nrow=3)
    torchvision.utils.save_image(g, str(logs_dir / f'{epoch:04}.png'))
    return total_loss / len(loader)

def train(batch_size: int = 16, epochs: int = 1, lr: float = 1.0e-3):
    device = torch.device('cuda') if torch.cuda.is_available() else torch.device('cpu')
    mobile_net = torchvision.models.mobilenet_v2(
        weights=torchvision.models.MobileNet_V2_Weights.IMAGENET1K_V2
    )
    net = Net(backbone=mobile_net.features, num_features=1280)
    net.to(device)
    optimizer = torch.optim.AdamW(net.head().parameters(), lr=lr)
    out_dir = Path('out')
    logs_dir = out_dir / 'logs'
    logs_dir.mkdir(exist_ok=True)
    # remove old logs
    for existing in logs_dir.glob('*'):
        existing.unlink()
    mask_size = (24, 24)
    mean=[0.485, 0.456, 0.406]
    std=[0.229, 0.224, 0.225]
    train_ds = Dataset(out_dir, 'train', mean=mean, std=std, mask_resize=mask_size)
    val_ds = Dataset(out_dir, 'val', mean=mean, std=std, augmentation=None, mask_resize=mask_size)
    train_loader = DataLoader(train_ds, batch_size=batch_size, shuffle=True)
    val_loader = DataLoader(val_ds, batch_size=1, shuffle=False)

    for epoch in range(epochs):
        net.head().train()
        train_loss = train_epoch(net, optimizer, train_loader, device)
        logger.info(f'[{epoch:04}]: train_loss={train_loss:.04f}')
        with torch.no_grad():
            net.eval()
            val_loss = validate(epoch, net, val_loader, device, logs_dir)
        logger.info(f'[{epoch:04}]: val_loss={val_loss:.04f}')

def main():
    train()

if __name__ == '__main__':
    main()