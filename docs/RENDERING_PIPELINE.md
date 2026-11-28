# Pipeline hiển thị M7

1. SceneRenderer ghép T × R × S của instance với local transform của submesh. Vertex shader chuyển đỉnh qua model, view, projection và truyền vị trí world, UV, normal cho fragment shader.
2. Color texture dùng GL_SRGB8 hoặc GL_SRGB8_ALPHA8. GL giải mã RGB lúc lấy mẫu, alpha giữ tuyến tính. Shader giải mã màu nền sRGB theo hàm từng đoạn IEC rồi nhân texture và cộng ánh sáng.
3. Fragment shader mã hóa kết quả sang sRGB một lần và ghi số màu hiển thị vào RGBA8. Các overlay dùng màu hiển thị trực tiếp. ImGui lấy mẫu texture RGBA8 bình thường. Default framebuffer không bật GL_FRAMEBUFFER_SRGB. Cùng quy ước áp dụng ở thumbnail. Depth24/stencil8 lưu độ sâu và stencil riêng với dữ liệu màu.
4. Lưới solid tính khoảng cách tới bội số gridSpacing trên XZ và đường biên phòng. fwidth đo khoảng cách phủ bởi một pixel theo từng trục, smoothstep làm dịu mép nét và giảm tương phản khi nhiều ô lọt vào một pixel. Lưới trộn lên mặt trên của sàn, nội thất vẫn che sàn qua depth test. Wireframe dùng mesh đường để giữ lưới khi tam giác sàn chỉ còn cạnh. Grid snapping vẫn dùng cùng khoảng cách theo mét.
5. Mặc định một mẫu. Khi chọn 2x/4x, Framebuffer kiểm tra GL_MAX_SAMPLES, cấp phát color/depth renderbuffer cùng số mẫu, thử mức thấp hơn nếu FBO incomplete. Active samples truy vấn từ renderbuffer. glBlitFramebuffer resolve màu với GL_NEAREST vào texture một mẫu trước khi ImGui vẽ. Picking dùng ray của camera và kích thước viewport. Resize tạo FBO thay thế trước khi giải phóng FBO cũ. Khi đóng ứng dụng, các GL handle được hủy trước context.

Ảnh đối chứng ở [báo cáo M7](M7_DELIVERY.md) dùng cùng camera. Tăng MSAA cải thiện biên hình học nhưng tăng bộ nhớ và băng thông. Lưới giải tích xử lý chi tiết trong tam giác ngay cả ở chế độ một mẫu.
