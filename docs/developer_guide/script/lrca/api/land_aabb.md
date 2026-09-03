# LandAABB 领地范围对象

前缀：`LandAABB_`。除特殊标注外，第一、二参数均为范围的两对角坐标 `a`/`b`（均为 `IntPos`，含 `dimid`）。

## 规整 AABB

`LandAABB_fix(a, b)` <Badge type="warning" text="deprecated v0.22.1" />  
`LandAABB_canonicalize(a, b)` <Badge type="tip" text="v0.22.1+" />

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: 规整后的范围 `[min, max]`（自动交换为 min/max 顺序）
- 返回值类型: `[IntPos, IntPos]`

## 获取 X 轴跨度

`LandAABB_getSpanX(a, b)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: X 轴坐标跨度
- 返回值类型: `Integer`

## 获取 Y 轴跨度

`LandAABB_getSpanY(a, b)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: Y 轴坐标跨度
- 返回值类型: `Integer`

## 获取 Z 轴跨度

`LandAABB_getSpanZ(a, b)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: Z 轴坐标跨度
- 返回值类型: `Integer`

## 获取平面面积

`LandAABB_getSquare(a, b)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: 平面面积（XZ 跨度相乘）
- 返回值类型: `Integer`

## 获取体积

`LandAABB_getVolume(a, b)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: 体积（XYZ 跨度相乘）
- 返回值类型: `Integer`

## 转为字符串

`LandAABB_toString(a, b)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: 格式化后的字符串描述
- 返回值类型: `String`

## 获取领地边框

`LandAABB_getBorder(a, b)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: 3D 边框上的所有整数坐标（立体矩形边框）
- 返回值类型: `IntPos[]`

## 获取领地范围点

`LandAABB_getRange(a, b)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: 平面矩形内的所有整数坐标点（仅 XZ）
- 返回值类型: `IntPos[]`

## 获取平面顶点

`LandAABB_getVertices(a, b)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: 4 个平面顶点（FloatPos，含 `dimid`）
- 返回值类型: `FloatPos[]`

## 获取立方体角点

`LandAABB_getCorners(a, b)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: 8 个立方体角点（FloatPos，含 `dimid`）
- 返回值类型: `FloatPos[]`

## 获取立方体边线

`LandAABB_getEdges(a, b)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
- 返回值: 12 条边线，每条边线由 2 个 `IntPos` 组成的数组
- 返回值类型: `[IntPos, IntPos][]`

## 判断坐标是否在范围内

`LandAABB_hasPos(a, b, pos, includeY)`

- 参数:
  - a : `IntPos`
    坐标点 A
  - b : `IntPos`
    坐标点 B
  - pos : `IntPos`
    要判断的坐标
  - includeY : `Boolean`
    是否校验 Y 轴
- 返回值: 该坐标是否在 AABB 内
- 返回值类型: `Boolean`

## 判断两个 AABB 是否碰撞

`LandAABB_isCollision(a, b, c, d)`

- 参数:
  - a : `IntPos`
    范围 1 min
  - b : `IntPos`
    范围 1 max
  - c : `IntPos`
    范围 2 min
  - d : `IntPos`
    范围 2 max
- 返回值: 是否碰撞（重合）
- 返回值类型: `Boolean`

## 判断两个 AABB 是否满足最小间距

`LandAABB_isComplisWithMinSpacing(a, b, c, d, minSpacing, includeY)`

- 参数:
  - a : `IntPos`
    范围 1 min
  - b : `IntPos`
    范围 1 max
  - c : `IntPos`
    范围 2 min
  - d : `IntPos`
    范围 2 max
  - minSpacing : `Integer`
    最小间距
  - includeY : `Boolean`
    是否包含 Y 轴
- 返回值: 是否满足最小间距要求
- 返回值类型: `Boolean`

## 判断是否包含另一 AABB

`LandAABB_isContain(a, b, c, d)`

- 参数:
  - a : `IntPos`
    源范围（包含方）min
  - b : `IntPos`
    源范围（包含方）max
  - c : `IntPos`
    被包含范围 min
  - d : `IntPos`
    被包含范围 max
- 返回值: 源范围 `(a,b)` 是否完整包含 `(c,d)`
- 返回值类型: `Boolean`
