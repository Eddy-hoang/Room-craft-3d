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

# Quyết định kỹ thuật M9 (`v1.0.0-rc1`)

- **Giữ & Gia cố JSON Parser nội bộ**: Thay vì thay bằng `nlohmann/json`, đã kiểm toán và gia cố triệt để `src/persistence/Json.cpp` (UTF-8 strict, từ chối lone low surrogate & duplicate key, `std::from_chars` độc lập locale, giới hạn `250,000` node) và kiểm chứng bằng `1,500` ca fuzzing determinstic (`M9_ReliabilityUnitTests` PASS).
- **Exact Dirty-State bằng chữ ký chuẩn hóa**: So sánh bản chuẩn hóa trạng thái được lưu trong `ProjectManager::captureSavedBaseline()` / `syncDirtyWithState()` thay vì chỉ dựa vào chỉ số ngăn xếp `CommandHistory`, giúp Undo về bản đã lưu trả `dirty = false` đồng thời vẫn nhận diện chính xác thay đổi ánh sáng/phòng/render ngoài Undo stack.
- **Chấp thuận Directional Shadow Mapping (M9.4)**: Sau khi đo benchmark Release trên Intel UHD Graphics cho thấy chi phí Directional Shadow Map `1024×1024` (3×3 PCF) chỉ là **`+0.61 ms`** (`2.12 ms → 2.73 ms`, giữ **`366.5 FPS`**, tương thích MSAA 4× ở **`292.2 FPS`**) và tăng rõ rệt chất lượng thị giác, quyết định giữ bật mặc định kèm công tắc & tùy chọn `512/1024/2048` trong `Room Settings`. `M7_LightingGpu` vẫn pass 100% nhờ mặc định shader `uShadowEnabled = 0`.
- **Tách biệt `preferences.json` khỏi `.roomcraft`**: Tùy chọn UI (`PreferencesManager`) lưu riêng theo người dùng tại `%APPDATA%\RoomCraft3D\preferences.json`, tắt `imgui.ini` để không làm bẩn thư mục làm việc hoặc gói portable chỉ đọc.
- **Giữ `Rotate/Scale Visual Gizmos` (`M9-F18`) ở trạng thái `DEFERRED`**: Không đánh đổi tiến độ và độ ổn định của Release Candidate vì tương tác `R`/`S` + trục `X/Y/Z` + Inspector đã đầy đủ và kiểm chứng.


---

## Quyết định Kiến trúc — M9 Visual Quality Refinement (`v1.0.0-rc2`)

- **DEC-RC2-01 (In-Shader ACES Tone Mapping thay vì HDR FBO riêng)**: Thực hiện ACES Filmic Tone Mapping trên biến `vec3 rgb` float tuyến tính ngay trong `mesh.frag` trước `displayColor()` giúp xử lý đầy đủ HDR không clipping mà không tốn thêm pass resolve `GL_RGBA16F` trên Intel UHD, giữ nguyên tương thích MSAA 2×/4× và không tác động lên Dear ImGui/gizmo/M7 linear tests (`uToneMapEnabled = 0` mặc định).
- **DEC-RC2-02 (Front-Face Culling có chọn lọc cho tường đặc + Double-Sided cho nội thất)**: Chỉ bật `GL_CULL_FACE(GL_FRONT)` cho 4 khối hộp tường đặc khép kín trong shadow pass để triệt tiêu wall shadow acne mà không làm mất bóng của chi tiết nội thất mỏng/hở (chụp đèn, lá cây).
- **DEC-RC2-03 (Analytical Floor Contact Grounding thay cho SSAO toàn màn hình)**: Dùng OBB signed distance trên sàn `Y = 0` (tối đa 16 vật thể đứng sàn, chỉ giảm nhẹ ambient) để giữ nhẹ trên Intel UHD, tránh quầng đen và tránh bóng giả dưới đèn bàn/máy tính.
- **DEC-RC2-04 (Giữ ổn định 14 Asset ID & Schema v1)**: Chỉ rút gọn `displayName` hiển thị cho 6 mẫu C++, thêm `room.showWindow` tùy chọn để bảo đảm tương thích ngược 100% với tệp `.roomcraft` từ M8 và RC1.
