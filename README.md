# HEIF-Utility-Native-DLL
<a href="https://github.com/liuziangexit/HEIF-Utility">HEIF-Utility</a> 的一部分。<br>
<a href="https://github.com/liuziangexit/HEIF-Utility-Native-DLL/blob/master/Srcs/HUD/main.cpp">主要源代码</a><br>

<h2>总览</h2>
此工程生成 HUD.dll，该动态库是 HEIF 实用工具的一部分。<br>

<h2>接口</h2>
void heif2jpg(const char heif_bin[], int input_buffer_size, const int jpg_quality, char output_buffer[], int output_buffer_size, const char* input_temp_filename)<br><br>
作用: 将 Apple HEIF 转换为 JPEG<br>
调用约定: Cdecl<br>
参数 heif_bin: HEIF 文件二进制数据<br>
参数 input_buffer_size: HEIF 文件二进制数据大小<br>
参数 jpg_quality: 设定输出的 JPEG 图像的质量。取值 1-100，数值越高质量越好<br>
参数 output_buffer: JPEG 图像输出缓冲区<br>
参数 output_buffer_size: 输出缓冲区大小<br>
参数 input_temp_filename: 函数运行时将会产生临时文件，通过此参数指定临时文件名。在执行结束后，临时文件可以被删除。<br>
异常: 无<br>

<h2>提示</h2>
如果编译出来的代码在 read_hevc_bitstream_to_mat_vector 那里提示无法打开 bitstream 文件，这说明你编译的 opencv 库不支持 h265 解码。在 Windows 上你只需要从官网下一个完整编译的二进制动态库，然后把那几个动态库放到根目录就可以成功打开了；但是如果在 linux/macOS 上出现这个问题，那情况就比较麻烦，你需要自己编译 opencv，使它支持 h265。

<h2>Apple HEIF 如何储存一张图片</h2>
Apple HEIF 将图片分割为数个较小的图块(tiles)，小图块的分辨率一般是 512*512，然后按照 从左到右，从上到下 的顺序，依次将图块存入 HEIF 图像序列。<br>
图块的角度可能和照片的角度不一致，但 HEIF 同时记录了图块相对于照片的角度。<br>
某些边缘的图块可能包含黑色填充区，因为某些时候照片的分辨率不一定就是 512*512 的倍数，所以多出来的部分用黑色填充。<br>

<h2>示例</h2>
对于一张由 iPhone 7 拍摄的分辨率为 4032*3024 的典型图像，Apple HEIF 将这样进行储存：<br>
图片被分割为 8*6 个图块，每行 8 个，一共 6 行。<br>
图片右边和下面的边缘处有填充，填充的像素数: 512*512*8*6-4032*3024=390144。<br><br>
<img src="/Images/img0.jpg"><br>

<h2>实现</h2>
源代码：https://github.com/liuziangexit/HEIF-Utility-Native-DLL/blob/master/Srcs/HUD/main.cpp <br><br>
1.提取 HEIF 的 宽度、高度、行数、列数、相对角度。(第 67 行，bool read_heif_info(heifdata&, HevcImageFileReader&, const uint32_t&)<br>
2.提取 HEIF 参数集。(第 95 行，bool read_heif_paramset(heifdata&, HevcImageFileReader&, const uint32_t&, const HevcImageFileReader::IdVector&)<br>
3.提取 HEIF 中所有的图块。(第 114 行，bool read_heif_tiles(heifdata&, HevcImageFileReader&, const uint32_t&, const HevcImageFileReader::IdVector&)<br>
4.把所有图块转换成 HEVC Bitstream(HEVC 裸流)，写入到文件。(第 161 行，bool write_hevc_bitstream(const std::string&, const heifdata&)<br>
5.从 HEVC Bitstream 中读取所有的帧(HEIF 中所有的图块)。(第 179 行，std::vector read_hevc_bitstream_to_mat_vector(const std::string&)<br>
6.把图块们拼起来。(第 281 行<br>
7.剪裁掉多余的黑色填充区域。(第 296 行<br>
8.旋转到正确角度。(第 299 行<br>
9.将 Bitmap 编码为 JPEG。(第 302 行<br>
<h2>踩坑</h2>
1.nokiatech 给的 hevcimagefilereader 无法正确读取第七个图块。<br>
详细描述：https://stackoverflow.com/questions/45485622/corrupted-heic-tile-when-converting-to-jpeg <br>
原因：该图块的 header 写入时使用了错误的 SPS；<br>
解决：https://github.com/liuziangexit/heif <br><br>
2.OpenCV 的 VideoCapture 只支持从文件或者摄像头读取视频流（而不能从内存直接读），这样就需要把裸流写到临时文件，然后再读那个临时文件。。。这完全是多此一举，磁盘 IO 是很慢的，拖累了整个函数的性能，目前的性能瓶颈就在这。<br>
如何解决：我在堆栈溢出上面找到了这个六年前的问题：https://stackoverflow.com/questions/5237175/process-video-stream-from-memory-buffer ，里面有个回答提到了如何通过修改 VideoCapture 代码来让它支持从内存中读取视频流的方法。我还没去着手做这件事情，等有空了再优化一波。
