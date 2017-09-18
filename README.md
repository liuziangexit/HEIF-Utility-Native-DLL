# HEIF-Utility-Native-DLL
Part Of <a href="https://github.com/liuziangexit/HEIF-Utility">HEIF-Utility</a>.<br>

<h2>总览</h2>
此工程生成 HUD.DLL，该动态库是 HEIF 实用工具的一部分。<br>

<h2>接口</h2>
void heif2jpg(const char* heif_bin, int input_buffer_size, const int jpg_quality, char* output_buffer, int output_buffer_size)<br>
作用: 将 Apple HEIF 转换为 JPEG。<br>
调用约定: Cdecl<br>
参数 heif_bin: HEIF 文件二进制数据。<br>
参数 input_buffer_size: HEIF 文件二进制数据大小。<br>
参数 jpg_quality: 设定输出的 JPEG 图像的质量。取值 1-100，数值越高质量越好。<br>
参数 output_buffer: JPEG 图像输出缓冲区。<br>
参数 heif_bin: 输出缓冲区大小。<br>
异常: 无。<br>

<h2>关于实现</h2>
