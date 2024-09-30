from pypinyin import pinyin, Style
import hanzi
from PIL import ImageFont, ImageDraw, Image
import numpy as np


def get_bitmap(char, size=16):
    # 创建一个空白图像
    img = Image.new('1', (size, size), 0)
    draw = ImageDraw.Draw(img)
    # 加载字体
    font = ImageFont.truetype(r"C:/Windows/Fonts/simsun.ttc", 16)
    # 渲染汉字到图像
    draw.text((0, 0), char, font=font, fill=1)
    return img

def bitmap_to_hex(bitmap):
    # 将图像转换为16*16数组
    data = np.array(bitmap).reshape(-1)
    # 输出列表
    hex_data = [np.dot(np.array([128,64,32,16,8,4,2,1]), data[i*8:i*8+8]) for i in range(32)]
    return hex_data


initial_list = ["", "b", "p", "m", "f", "d", "t", "n", "l", "g", 
              "k", "h", "j", "q", "x", "zh", "ch", "sh", "r", "z",
              "c", "s", "y", "w"]  # 24
initial_list.sort()

final_list = ["a", "o", "e", "i", "u", "v", "ai", "ei", "ui", "ao",
              "ou", "iu", "ie", "ue", "er", "an", "en", "in", "un",  # ve,vn合并到ue,un
              "ang", "eng", "ing", "ong", "ia", "iao", "ian", "iang", "iong", "ua",
              "uai", "uan", "uang", "uo"]  # 33
final_list.sort()


character = [[] for i in range(24*33)]
character_utf8 = [[] for i in range(24*33)]
character_len = []
character_offset = []
character_bitmap = []

def pinyin_encode(ch):
    index = []
    # 不采用分别获取声母韵母的方法, 因为部分拼音返回与实际输入法使用不同(例如"研" yan->i an)
    pych = pinyin(ch, style=Style.NORMAL, heteronym=True)[0]  # 返回str
    if pinyin(ch, style=Style.FINALS) == [[""]]:  # 排除"兙"等字
        return -1
    for py in pych:
        # 无声母
        if py[0] in "aoeiuv":
            initial = ""
            final = py
        elif py[0:2] in "zhchsh":
            initial = py[0:2]
            final = py[2:]
        else:
            initial = py[0]
            final = py[1:]
        # 替换
        if final == "ve":
            final = "ue"
        if final == "vn":
            final = "un"
        # 寻找下标
        try:
            index.append(initial_list.index(initial) * 33 + final_list.index(final))
        except:
            # print(ch)
            # print(py)
            # print(ch.encode("utf-8"))
            pass
    return index

# 遍历所有汉字, 并按读音分组
b = bytearray(b'aaa')
cnt = 0
for i in range(0x4e00, 0x9fa6):
    # 转换utf8编码
    b[0] = 0xe0 | ((i & 0xf000) >> 12)
    b[1] = 0x80 | ((i & 0xfc0) >> 6)
    b[2] = 0x80 | (i & 0x3f)
    # print(pinyin(b.decode(), Style.initial_LETTER, heteronym=False))
    index = pinyin_encode(b.decode())
    if index != -1:
        for idx in index:
                character[idx].append(b.decode())
                character_utf8[idx].append(i)  


# 按笔画排序
hz = hanzi.Hanzi()
def get_bihua_safe(ch):
    try:
        return (hz.get_bihua(ch)[0].__len__() + 1) // 2
    except:  # hanzi库部分汉字无法返回笔画, 例如"呃"
        return 99


for i in range(24*33):
    if(character[i].__len__() > 0):
        zipped = list(zip(character[i], character_utf8[i]))
        zipped_sorted = sorted(zipped, key = lambda ch: get_bihua_safe(ch[0]))
        character[i], character_utf8[i] = zip(*zipped_sorted)
        character[i] = list(character[i])
        character_utf8[i] = list(character_utf8[i])

# # 输出所有文字
# for i in range(24*33):
#     print(initial_list[i // 33])
#     print(final_list[i % 33])
#     print(character[i])
# exit(0)

# 输出各拼音在字库中存储的长度与偏移量
sum = 0
for i in range(24*33):
    character_offset.append(sum)
    character_len.append(character[i].__len__())
    sum += character[i].__len__()
# print(character_offset)
# print(character_len)
# exit(0)


character_merged = "".join(["".join(s) for s in character])

# 输出所有汉字的unicode
character_utf8_merged = []
for s in character_utf8:
    character_utf8_merged = character_utf8_merged + s

for PAGE in range(sum // 16384 + 1):
    if PAGE == 0:
        print("#if FONTTYPE == 30", "\nuint8_t fonttype[] = {")
    else:
        print("#elif FONTTYPE ==", PAGE + 30, "\nuint8_t fonttype[] = {")
    character_merged_page = character_utf8_merged[PAGE * 1024: (PAGE + 1) * 1024]
    for i in range(character_merged_page.__len__()):
        print(character_merged_page[i] // 256, end=",")
        print(character_merged_page[i] % 256, end=",")
        if i % 20 == 19:
            print("")
    print("};")
print("#endif")

# # 输出所有汉字的点阵, 每1024字为一部分
# # 运行约4min
# character_bitmap_merged = []
# # PAGE = 0
# for PAGE in range(sum // 1024 + 1):
#     if PAGE == 0:
#         print("#if FONTTYPE == 0", "\nuint8_t fonttype[] = {")
#     else:
#         print("#elif FONTTYPE ==", PAGE, "\nuint8_t fonttype[] = {")
#     character_merged_page = character_merged[PAGE * 1024: (PAGE + 1) * 1024]
#     for s in character_merged_page:
#         bitmap = get_bitmap(s)
#         hex_data = bitmap_to_hex(bitmap)
#         for i in range(hex_data.__len__()):
#             print(hex_data[i], end=",")
#         print("")
#         character_bitmap_merged = character_bitmap_merged + hex_data
#     print("};")
# print("#endif")