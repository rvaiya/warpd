sudo cp warpd.service /etc/systemd/user/warpd.service

systemctl --user daemon-reload
systemctl --user enable warpd.service
systemctl --user start warpd.service
