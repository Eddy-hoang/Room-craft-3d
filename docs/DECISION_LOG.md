# Quyết định kỹ thuật M7

- Dùng forward rendering và hai nguồn trực tiếp cố định để giữ pipeline OpenGL 3.3 đơn giản. Preset là các giá trị CPU. Thay preset không làm mất thumbnail cache.
- Dùng sRGB texture và mã hóa kết quả trong mesh fragment shader. Texture trình bày của ImGui là RGBA8, nhờ vậy palette UI và overlay sử dụng trực tiếp mã màu hiển thị.
- Vẽ lưới solid trên chính sàn sau phép đo bias ở M6. Giữ mesh đường cho wireframe. Đối chứng và ảnh gốc nằm trong [báo cáo](M7_DELIVERY.md).
- MSAA mặc định Off cho Intel UHD, cho phép 2x/4x theo driver. P2 directional shadow hoãn sau gate vì p95 editor ở cấu hình Off đã vượt ngân sách 16,67 ms trong một lần đo đối chứng (lần cuối đạt 7,36 ms).

# Quyết định kỹ thuật M8

- **JSON Serialization không thêm phụ thuộc ngoài**: Sau khi đánh giá `nlohmann/json`, chọn bộ phân tích/xuất JSON RFC 8259 nội bộ (`src/persistence/Json.cpp`) nhằm giữ tốc độ biên dịch nhanh, hoạt động 100% offline (`-DFETCHCONTENT_UPDATES_DISCONNECTED=ON`), kiểm soát chặt giới hạn dung lượng (`16 MB`), độ sâu (`64`), phân biệt `int64`/`uint64` và từ chối `NaN`/`Infinity` hay ký tự thừa.
- **Lossless `uint64_t` ObjectId**: Mã hóa `objectId` và `nextObjectId` dạng chuỗi thập phân (`"1"`, `"9007199254740993"`) trong `.roomcraft` schema v1 để tránh mất độ chính xác IEEE-754 double vượt quá $2^{53}$, đồng thời kiểm tra nghiêm ngặt khi nạp.
- **Collision Detection 2 giai đoạn + chế độ mặc định `Warning Only`**: Kết hợp Broad-Phase AABB và Narrow-Phase 15-axis 3D OBB SAT với dung sai `5 mm` (phân biệt `Touching` và `Penetrating`). Chọn `Warning Only` làm mặc định và bỏ qua cặp nội thất–thảm (`allowRugOverlap = true`) cùng vật ẩn (`includeHiddenObjects = false`) để tránh cảnh báo giả khi đặt đồ lên thảm hoặc đèn trên bàn.
- **Safe Atomic Saving & Transactional Loading**: Ghi ra tệp tạm trong cùng thư mục đích rồi thay thế bằng `MoveFileExW(..., MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` trên Windows; nạp tệp qua `ProjectData` + `Scene` ứng viên rồi mới commit để không bao giờ hỏng tệp cũ hay làm mất cảnh đang mở khi gặp lỗi.
- **Native Win32 File Dialog & `WM_CLOSE` Intercept**: Dùng `comdlg32` (`GetOpenFileNameW`/`GetSaveFileNameW` với `OFN_NOCHANGEDIR`) và subclass `WM_CLOSE` trong `Window.cpp` để đồng bộ hộp thoại `Save / Discard / Cancel` khi đóng cửa sổ hệ điều hành mà không đổi stack FreeGLUT/ImGui.
- **Phạm vi tùy chọn M8**: Đã triển khai `Recent Projects` (P1) và bounded `CommandHistory` Undo/Redo 64 bước (P2); hoãn `UI Layout Persistence` (P2 — `M8-F18`) sang M9 để ưu tiên sự ổn định của Collision và Project Persistence. Shadow Mapping tiếp tục giữ `DEFERRED`.
