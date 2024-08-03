var isDarkMode = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches
window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', e => {
    applyDarkMode(e.matches)
})

const applyDarkMode = (dark = false) => {
    document.getElementsByTagName('html')[0].setAttribute('data-bs-theme', dark ? 'dark' : 'light')
}
applyDarkMode(isDarkMode)


const enurl = encodeURIComponent

const goTo = (url) => location.href = location.href.split('/').slice(0, -1).join('/') + url


const isMobile = window.innerWidth < 768

const { Loading, Confirm } = Notiflix

Confirm.init({ width: '400px', titleColor: 'rgb(var(--bs-primary-rgb))', okButtonBackground: 'rgb(var(--bs-primary-rgb))', titleFontSize: '1.3em', messageColor: 'var(--bs-body-color)', messageFontSize: '1em', buttonFontSize: '1em' })
Loading.init({ svgColor: 'var(--bs-body-color)' })
Notiflix.Notify.init({ useFontAwesome: true, position: isMobile ? 'center-bottom' : 'right-top', timeout: isMobile ? 3000 : 10000, width: '350px', fontSize: '1.1em' })

const $main = document.getElementById('main')

const GET = (url) => fetch(url).then(response => response.json())
const POST = (url, data) => fetch(url, { method: 'POST', body: data }).then(response => response.json())

const emptyNode = () => {
    $main.querySelector('.col-node-empty').classList.remove('d-none')
}

const notify = (type, ...message) => {
    Notiflix.Notify[type](message.join(' '))
}

const alertReload = (title = "Error", message = "") => {
    Confirm.show(title, message, 'Reload', 'Close', () => { location.reload() });
}

const json2query = (json) => {
    return Object.keys(json).map(key => enurl(key) + '=' + enurl(json[key])).join('&')
}