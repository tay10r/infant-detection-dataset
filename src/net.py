from torch import Tensor, nn
from torch.nn import functional as F

class _Head(nn.Module):
    def __init__(self, num_features: int):
        super().__init__()
        self.__layers = nn.Sequential(
            nn.Conv2d(num_features, 32, kernel_size=1),
            nn.Conv2d(32, 16, kernel_size=3, padding=1),
            nn.Conv2d(16, 1, kernel_size=1)
        )

    def forward(self, x: Tensor) -> Tensor:
        x = self.__layers(x)
        return x

class Net(nn.Module):
    def __init__(self, backbone: nn.Module, num_features: int):
        super().__init__()
        self.__backbone = backbone
        self.__head = _Head(num_features)

    def head(self) -> nn.Module:
        return self.__head

    def forward(self, x: Tensor) -> Tensor:
        x = self.__backbone(x)
        x = self.__head(x)
        return x