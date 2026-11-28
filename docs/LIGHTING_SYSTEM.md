# Hệ thống chiếu sáng M7

Shader mesh nhận một nguồn môi trường, tối đa một đèn hướng và một đèn điểm. Giới hạn này giữ số phép tính mỗi fragment cố định trên Intel UHD. Mọi vị trí và hướng trong phép tính đều ở world space. Đèn hướng dùng L = normalize(−direction). Đèn điểm dùng L = normalize(position − P), d = |position − P|. Với camera trực giao, hướng nhìn V giống nhau trên cả vật thể. Với camera phối cảnh, V phụ thuộc vị trí fragment P.

Với màu nền tuyến tính B, ánh sáng môi trường là B × ambient. Mỗi nguồn bổ sung I × (B × max(N·L,0) + Ks × max(N·H,0)^Ns), với H = normalize(L+V). Cả thành phần khuếch tán và phản xạ chỉ xuất hiện khi N·L > 0. I bằng màu đèn nhân cường độ. Đèn điểm chia I cho max(kc + kl d + kq d², 0,001). Mẫu số được chặn và các vector độ dài 0 trả về vector 0. Nguồn bị tắt gửi radiance 0.

| Preset | Ambient | Hướng tia | Màu sun RGB, cường độ | Point, cường độ |
|---|---:|---|---|---|
| Daylight | 0,42 | −0,6 / −1 / −0,45 | (1 / 0,95 / 0,86) × 0,72 | tắt, 1,3 |
| Warm Interior | 0,32 | như Daylight | như Daylight × 0,36 thay cho 0,72 | bật, 2 |
| Neutral Studio | 0,50 | như Daylight | (1 / 1 / 1) × 0,62 | tắt, 1,3 |
| Evening | 0,17 | −1 / −0,28 / 0,4 | (0,48 / 0,62 / 1) × 0,35 | bật, 1,3 |

Đèn điểm có vị trí ban đầu (−1 / 2,3 / 0,7) m, màu (1 / 0,68 / 0,42), hệ số suy giảm (1 / 0,18 / 0,12). Giá trị RGB của đèn là hệ số tuyến tính. Room Settings cập nhật uniform ở khung tiếp theo. Thay đổi ánh sáng không làm tăng revision hình học. Ma trận suy biến gửi pháp tuyến 0, để bề mặt còn thành phần môi trường. Đèn không xét vật cản trong M7.
