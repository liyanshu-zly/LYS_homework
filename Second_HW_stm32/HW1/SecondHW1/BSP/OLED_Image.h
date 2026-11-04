#ifndef __OLED_IMAGE_H
#define __OLED_IMAGE_H

#include "main.h"

/* ==========================================================================
 * 石头 / 剪刀 / 布 图标 (SSD1306 128x64)
 *
 * 数据格式: 页优先, 每页 W 字节, bit0 = 该页最上面一行, 和 OLED_F8x16 字模
 *           完全一样, 可以直接喂给 OLED_WriteData()。
 *
 *   RPS_Icon48_x : 48x48, 6 页 x 48 字节 = 288 字节
 *   RPS_Icon32_x : 32x32, 4 页 x 32 字节 = 128 字节
 *   RPS_CN16_x   : 16x16 汉字(石头/剪刀/布), 2 页 x 16 字节
 *
 * 全部绘制函数按"页"对齐: Page 是页地址(0~7), 即 y = Page * 8。
 *   48x48 图标占 6 页  -> Page 取 0~2
 *   32x32 图标占 4 页  -> Page 取 0~4
 *   16x16 汉字占 2 页  -> Page 取 0~6
 * ========================================================================== */

#define RPS_ROCK      0
#define RPS_SCISSORS  1
#define RPS_PAPER     2

/* 48x48 大图 */
extern const uint8_t RPS_Icon48_Rock[288];
extern const uint8_t RPS_Icon48_Scissors[288];
extern const uint8_t RPS_Icon48_Paper[288];

/* 32x32 小图(三个并排也放得下) */
extern const uint8_t RPS_Icon32_Rock[128];
extern const uint8_t RPS_Icon32_Scissors[128];
extern const uint8_t RPS_Icon32_Paper[128];

/* 16x16 汉字 */
extern const uint8_t RPS_CN16_ShiTou[64];      /* 石头 */
extern const uint8_t RPS_CN16_JianDao[64];     /* 剪刀 */
extern const uint8_t RPS_CN16_Bu[32];          /* 布   */

/* 按 id(RPS_ROCK / RPS_SCISSORS / RPS_PAPER) 取数据 */
const uint8_t *RPS_GetIcon48(uint8_t id);
const uint8_t *RPS_GetIcon32(uint8_t id);
const uint8_t *RPS_GetCN16(uint8_t id);

/* 底层绘制 */
void OLED_ShowCN16(uint8_t X, uint8_t Page, const uint8_t *Bmp);
void OLED_DrawIcon48(uint8_t X, uint8_t Page, const uint8_t *Bmp);
void OLED_DrawIcon32(uint8_t X, uint8_t Page, const uint8_t *Bmp);

/* 汉字标签: CenterX = 标签中心的列像素, Page = 标签所在页 */
void OLED_ShowRPSLabel(uint8_t id, uint8_t CenterX, uint8_t Page);

/* 现成布局 ---------------------------------------------------------------- */
/* 两个 48x48 大图并排(占 y=0~47), 汉字标签在第 6 页(y=48~63) */
void OLED_ShowRPSPair(uint8_t leftId, uint8_t rightId);

/* 三个 32x32 小图横排(占 y=0~31), 汉字标签在第 5 页(y=40~55) */
void OLED_ShowRPSRow(uint8_t id0, uint8_t id1, uint8_t id2);

/* 单个 48x48 大图居中 + 标签居中 */
void OLED_ShowRPSSingle(uint8_t id);

/* 不闪的刷新方式 ---------------------------------------------------------- */
/* 局部清屏: 把 X 起 W 列、Page 起 Pages 页的矩形清成 0 */
void OLED_ClearArea(uint8_t X, uint8_t Page, uint8_t W, uint8_t Pages);

/* 不调 OLED_Clear() 的刷新版: 图形数据自带背景(0), 直接覆盖刷新过的区域,
   屏幕上不会出现"先整屏一黑再画出来"的过程, 所以不闪。
   注意: 它只覆盖图标和标签所占的区域, 屏幕上别处的内容不会被动到 */
void OLED_ShowRPSPairFast(uint8_t leftId, uint8_t rightId);
void OLED_ShowRPSRowFast(uint8_t id0, uint8_t id1, uint8_t id2);

/* 只在内容变化时才刷新(内部记住上一次的 id), 主循环里可以放心一直调用 */
void RPS_UpdatePair(uint8_t leftId, uint8_t rightId);
void RPS_UpdateRow(uint8_t id0, uint8_t id1, uint8_t id2);

#endif
